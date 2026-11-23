#include "render/render_thread.h"

#include <SDL3/SDL_vulkan.h>
#include <cmath>

#include "core/check.h"
#include "core/log.h"

namespace ih::render {

namespace {

constexpr vk::DeviceSize ARENA_SIZE = 256ull * 1024 * 1024;
constexpr vk::DeviceSize RING_SLOT_SIZE = 16ull * 1024 * 1024;
constexpr int INGEST_BUDGET = 64;          // meshes per frame, avoids hitches
constexpr uint64_t COLLECT_INTERVAL = 60;  // frames between unload sweeps

std::vector<std::string> requiredInstanceExtensions () {
    uint32_t count = 0;
    const char* const* names = SDL_Vulkan_GetInstanceExtensions (&count);
    return {names, names + count};
}

}  // namespace

RenderThread::RenderThread (platform::Window& window, core::FrameParamsQueue& params, const core::Config& config)
    : m_window (window), m_params (params), m_config (config) {}

RenderThread::~RenderThread () {
    stop ();
}

void RenderThread::start () {
    IH_CHECK (!m_running);
    m_running = true;
    m_thread = std::thread ([this] { loop (); });
}

void RenderThread::stop () {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable ()) m_thread.join ();
}

void RenderThread::ingestMeshes (uint32_t frame) {
    world::ChunkCoord coord{};
    std::vector<world::MeshData> parts;
    const auto& dims = m_config.render.chunk;

    for (int budget = 0; budget < INGEST_BUDGET; ++budget) {
        if (!m_world->takeReadyMesh (coord, parts)) break;
        m_worldRenderer->removeDraws (coord);

        for (auto& part : parts) {
            if (part.empty ()) continue;
            const vk::DeviceSize vertexBytes = part.vertices.size () * sizeof (uint64_t);
            const vk::DeviceSize indexBytes = part.indices.size () * sizeof (uint16_t);
            const auto vertexOffset = m_arena->allocate (vertexBytes);
            const auto indexOffset = m_arena->allocate (indexBytes);
            if (!vertexOffset || !indexOffset) {
                core::logError ("Arena exhausted; chunk mesh dropped");
                if (vertexOffset) m_arena->free (*vertexOffset, vertexBytes);
                continue;
            }
            const vk::DeviceSize stagingVertices = m_ring->write (frame, part.vertices.data (), vertexBytes);
            const vk::DeviceSize stagingIndices = m_ring->write (frame, part.indices.data (), indexBytes);

            std::array<vk::BufferCopy, 2> copies = {
                vk::BufferCopy (stagingVertices, *vertexOffset, vertexBytes),
                vk::BufferCopy (stagingIndices, *indexOffset, indexBytes),
            };
            m_swapchain->commandBuffer (frame).copyBuffer (m_ring->buffer (), m_arena->buffer (), copies);

            WorldRenderer::DrawRecord record{};
            record.coord = coord;
            record.vertexOffset = *vertexOffset;
            record.indexOffset = *indexOffset;
            record.indexCount = static_cast<uint32_t> (part.indices.size ());
            record.bounds.min = glm::vec3 (coord.x * dims.width, 0, coord.z * dims.depth);
            record.bounds.max = glm::vec3 ((coord.x + 1) * dims.width, dims.height, (coord.z + 1) * dims.depth);
            m_worldRenderer->addDraw (record);
        }
    }
}

void RenderThread::loop () {
    try {
#ifdef ENABLE_VK_VALIDATION
        constexpr bool validation = true;
#else
        constexpr bool validation = false;
#endif
        m_context = std::make_unique<vulkan::Context> (validation, requiredInstanceExtensions ());

        m_textures = std::make_unique<vulkan::TextureArray> ();
        m_textures->load (*m_context, "assets/textures/blocks", m_config.render.tileSize, m_config.render.mipLevels);

        m_faceLayers.resize (m_config.blocks.size ());
        for (size_t id = 1; id < m_config.blocks.size (); ++id) {
            for (size_t face = 0; face < 6; ++face) {
                m_faceLayers[id][face] =
                    static_cast<uint16_t> (m_textures->layerOf (m_config.blocks[id].faceTextures[face]));
            }
        }

        m_arena = std::make_unique<vulkan::Arena> ();
        m_arena->create (*m_context, ARENA_SIZE);
        m_ring = std::make_unique<vulkan::UploadRing> ();
        m_ring->create (*m_context, RING_SLOT_SIZE, vulkan::FRAMES_IN_FLIGHT);

        uint32_t width = 0, height = 0;
        m_window.getSize (width, height);
        m_swapchain = std::make_unique<vulkan::Swapchain> ();
        m_swapchain->init (*m_context, m_window.handle (), width, height);

        m_worldRenderer = std::make_unique<WorldRenderer> ();
        m_worldRenderer->setResources (*m_textures, *m_arena);
        m_worldRenderer->setChunkDims (m_config.render.chunk);
        m_worldRenderer->init (*m_context, m_swapchain->renderPass ());

        m_world = std::make_unique<world::World> (m_config, m_faceLayers);
        m_world->startWorkers ();

        core::FrameParams latest{};
        bool haveParams = false;
        while (m_running) {
            core::FrameParams incoming{};
            if (m_params.waitLatest (incoming, std::chrono::milliseconds (4))) {
                latest = incoming;
                haveParams = true;
            }
            if (!haveParams) continue;

            if (latest.width != m_swapchain->width () || latest.height != m_swapchain->height ()) {
                m_swapchain->recreate (latest.width, latest.height);
            }

            const auto& dims = m_config.render.chunk;
            const world::ChunkCoord center{static_cast<int> (std::floor (latest.cameraPos.x / dims.width)),
                                           static_cast<int> (std::floor (latest.cameraPos.z / dims.depth))};
            m_world->updateDesired (center, latest.renderDistance);
            if (m_frameCounter % COLLECT_INTERVAL == 0) {
                m_world->collectDead (center, latest.renderDistance);
            }

            if (!m_swapchain->beginFrame (m_frame)) continue;
            m_ring->resetSlot (m_frame);
            ingestMeshes (m_frame);
            m_swapchain->beginRenderPass (m_frame);

            FrameContext frame{m_swapchain->commandBuffer (m_frame), m_frame, m_swapchain->imageIndex (m_frame),
                               {m_swapchain->width (), m_swapchain->height ()}};
            m_worldRenderer->beginFrame (latest.viewProj, m_config.render.sunDirection, m_config.render.ambient,
                                         latest.frustum, latest.wireframe);
            m_worldRenderer->render (frame);

            m_swapchain->endFrame (m_frame);
            m_frame = (m_frame + 1) % vulkan::FRAMES_IN_FLIGHT;
            ++m_frameCounter;
        }

        m_context->device ().waitIdle ();
    } catch (const std::exception& e) {
        core::logError (std::string ("Render thread failed: ") + e.what ());
        m_running = false;
    }
    // Teardown: workers first, then GPU resources, then device/instance.
    m_world.reset ();
    m_worldRenderer.reset ();
    m_ring.reset ();
    m_arena.reset ();
    m_textures.reset ();
    m_swapchain.reset ();
    m_context.reset ();
}

}  // namespace ih::render
