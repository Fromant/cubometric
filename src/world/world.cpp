#include "world/world.h"

#include <algorithm>
#include <chrono>

#include "core/check.h"
#include "core/log.h"

namespace ih::world {

namespace {

bool stateAtLeast (ChunkState state, ChunkState minimum) {
    return static_cast<uint8_t> (state) >= static_cast<uint8_t> (minimum);
}

// Horizontal neighbor offsets, order +X, -X, +Z, -Z (matches NeighborChunks).
constexpr std::array<ChunkCoord, 4> SIDE_OFFSETS = {{
    {1, 0}, {-1, 0}, {0, 1}, {0, -1},
}};

}  // namespace

World::World (const core::Config& config, const FaceLayers& faceLayers)
    : m_config (config), m_faceLayers (faceLayers), m_generator (config.worldgen, config) {}

World::~World () {
    stopWorkers ();
}

void World::startWorkers () {
    IH_CHECK (!m_running);
    m_running = true;
    int count = m_config.render.workerThreads;
    if (count <= 0) {
        count = static_cast<int> (std::thread::hardware_concurrency ()) - 2;
    }
    count = std::clamp (count, 1, 16);
    for (int i = 0; i < count; ++i) {
        m_workers.emplace_back ([this] { workerLoop (); });
    }
    core::logInfo ("World: " + std::to_string (count) + " worker threads");
}

void World::stopWorkers () {
    if (!m_running) return;
    m_running = false;
    for (auto& worker : m_workers) worker.join ();
    m_workers.clear ();
}

void World::workerLoop () {
    while (m_running) {
        ChunkCoord coord{};
        if (m_genQueue.tryPop (coord)) {
            generateTask (coord);
        } else if (m_meshQueue.tryPop (coord)) {
            meshTask (coord);
        } else {
            std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
    }
}

void World::updateDesired (ChunkCoord center, int radius) {
    std::lock_guard lock (m_mapMutex);
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const ChunkCoord coord{center.x + dx, center.z + dz};
            auto it = m_chunks.find (coord);
            if (it == m_chunks.end ()) {
                it = m_chunks.emplace (coord, std::make_unique<Chunk> (m_config.render.chunk)).first;
            }
            Chunk& chunk = *it->second;
            ChunkState expected = ChunkState::Empty;
            if (chunk.state.compare_exchange_strong (expected, ChunkState::Generating)) {
                if (!m_genQueue.tryPush (coord)) {
                    chunk.state.store (ChunkState::Empty);  // retry next update
                }
            }
        }
    }
}

void World::generateTask (ChunkCoord coord) {
    Chunk* chunk = nullptr;
    {
        std::lock_guard lock (m_mapMutex);
        const auto it = m_chunks.find (coord);
        if (it == m_chunks.end ()) return;
        chunk = it->second.get ();
    }
    if (chunk->state.load () != ChunkState::Generating) return;

    m_generator.generate (coord, chunk->data);
    chunk->state.store (ChunkState::Generated);

    requestMesh (*chunk, coord);
    // Neighbors that already had a mesh must redo their boundary faces.
    std::lock_guard lock (m_mapMutex);
    for (const auto& offset : SIDE_OFFSETS) {
        const auto it = m_chunks.find ({coord.x + offset.x, coord.z + offset.z});
        if (it == m_chunks.end ()) continue;
        Chunk& neighbor = *it->second;
        ChunkState expected = ChunkState::Ready;
        if (neighbor.state.compare_exchange_strong (expected, ChunkState::Meshing)) {
            if (!m_meshQueue.tryPush ({coord.x + offset.x, coord.z + offset.z})) {
                neighbor.state.store (ChunkState::Ready);
            }
        }
    }
}

void World::requestMesh (Chunk& chunk, ChunkCoord coord) {
    ChunkState expected = ChunkState::Generated;
    if (chunk.state.compare_exchange_strong (expected, ChunkState::Meshing)) {
        if (!m_meshQueue.tryPush (coord)) {
            chunk.state.store (ChunkState::Generated);
        }
    }
}

void World::meshTask (ChunkCoord coord) {
    // Pin self + neighbors under the map mutex so collectDead cannot free
    // them while we read. Missing/Dead neighbors are treated as air.
    Chunk* chunk = nullptr;
    NeighborChunks neighbors{};
    {
        std::lock_guard lock (m_mapMutex);
        const auto it = m_chunks.find (coord);
        if (it == m_chunks.end ()) return;
        chunk = it->second.get ();
        if (chunk->state.load () != ChunkState::Meshing) return;
        chunk->refs++;
        for (size_t i = 0; i < SIDE_OFFSETS.size (); ++i) {
            const auto nit = m_chunks.find ({coord.x + SIDE_OFFSETS[i].x, coord.z + SIDE_OFFSETS[i].z});
            if (nit == m_chunks.end ()) continue;
            Chunk& neighbor = *nit->second;
            const ChunkState state = neighbor.state.load ();
            if (state == ChunkState::Dead || state == ChunkState::Empty || state == ChunkState::Generating) {
                continue;  // air for now; remeshed when it lands
            }
            neighbor.refs++;
            neighbors.sides[i] = &neighbor.data;
        }
    }

    auto mesh = meshChunk (chunk->data, neighbors, m_faceLayers);

    {
        std::lock_guard lock (m_mapMutex);
        chunk->refs--;
        for (size_t i = 0; i < SIDE_OFFSETS.size (); ++i) {
            if (!neighbors.sides[i]) continue;
            const auto nit = m_chunks.find ({coord.x + SIDE_OFFSETS[i].x, coord.z + SIDE_OFFSETS[i].z});
            if (nit != m_chunks.end ()) nit->second->refs--;
        }
        if (chunk->state.load () == ChunkState::Dead) return;
        chunk->mesh = std::move (mesh);
        chunk->state.store (ChunkState::Ready);
    }
    if (m_readyQueue.tryPush (coord)) {
        m_readyCount.fetch_add (1, std::memory_order_relaxed);
    }
}

bool World::takeReadyMesh (ChunkCoord& coord, std::vector<MeshData>& out) {
    if (!m_readyQueue.tryPop (coord)) return false;
    m_readyCount.fetch_sub (1, std::memory_order_relaxed);
    std::lock_guard lock (m_mapMutex);
    const auto it = m_chunks.find (coord);
    if (it == m_chunks.end ()) return false;
    Chunk& chunk = *it->second;
    if (chunk.state.load () != ChunkState::Ready) return false;
    out = std::move (chunk.mesh);
    chunk.mesh.clear ();
    return true;
}

void World::collectDead (ChunkCoord center, int radius) {
    std::lock_guard lock (m_mapMutex);
    const int keep = radius + 2;
    for (auto it = m_chunks.begin (); it != m_chunks.end ();) {
        const ChunkCoord coord = it->first;
        const bool outside = std::abs (coord.x - center.x) > keep || std::abs (coord.z - center.z) > keep;
        Chunk& chunk = *it->second;
        if (outside) {
            ChunkState expected = ChunkState::Ready;
            if (!chunk.state.compare_exchange_strong (expected, ChunkState::Dead)) {
                expected = ChunkState::Generated;
                chunk.state.compare_exchange_strong (expected, ChunkState::Dead);
            }
        }
        if (chunk.state.load () == ChunkState::Dead && chunk.refs == 0) {
            it = m_chunks.erase (it);
        } else {
            ++it;
        }
    }
}

}  // namespace ih::world
