#include "render/renderers/world_renderer.h"

#include <array>
#include <cstddef>

#include "core/check.h"
#include "render/vulkan/shader.h"

namespace ih::render {

void WorldRenderer::setResources (const vulkan::TextureArray& textures, const vulkan::Arena& arena) {
    m_textures = &textures;
    m_arena = &arena;
}

void WorldRenderer::createPipeline (vulkan::Context& context, vk::RenderPass renderPass, bool wireframe) {
    auto& device = context.device ();
    const auto vertModule = vulkan::loadShaderModule (device, "shaders/world.vert.spv");
    const auto fragModule = vulkan::loadShaderModule (device, "shaders/world.frag.spv");
    const std::array stages = {
        vk::PipelineShaderStageCreateInfo ({}, vk::ShaderStageFlagBits::eVertex, vertModule, "main"),
        vk::PipelineShaderStageCreateInfo ({}, vk::ShaderStageFlagBits::eFragment, fragModule, "main"),
    };

    const vk::VertexInputBindingDescription binding (0, sizeof (uint64_t), vk::VertexInputRate::eVertex);
    const vk::VertexInputAttributeDescription attribute (0, 0, vk::Format::eR32G32Uint, 0);
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 1;
    vertexInput.pVertexAttributeDescriptions = &attribute;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.polygonMode = wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    // The projection flips Y for Vulkan clip space, which reverses winding.
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;

    vk::PipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                   | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttachment;

    const std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t> (dynamicStates.size ());
    dynamicState.pDynamicStates = dynamicStates.data ();

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = static_cast<uint32_t> (stages.size ());
    pipelineInfo.pStages = stages.data ();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (wireframe) {
        m_wirePipeline = vk::raii::Pipeline (device, nullptr, pipelineInfo);
    } else {
        m_pipeline = vk::raii::Pipeline (device, nullptr, pipelineInfo);
    }
}

void WorldRenderer::init (vulkan::Context& context, vk::RenderPass renderPass) {
    IH_CHECK (m_textures && m_arena);
    auto& device = context.device ();

    const vk::DescriptorSetLayoutBinding binding (0, vk::DescriptorType::eCombinedImageSampler, 1,
                                                  vk::ShaderStageFlagBits::eFragment);
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    m_setLayout = vk::raii::DescriptorSetLayout (device, layoutInfo);

    const vk::DescriptorPoolSize poolSize (vk::DescriptorType::eCombinedImageSampler, 1);
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    m_pool = vk::raii::DescriptorPool (device, poolInfo);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &*m_setLayout;
    m_set = std::move (device.allocateDescriptorSets (allocInfo)[0]);

    const vk::DescriptorImageInfo imageInfo (m_textures->sampler (), m_textures->view (),
                                             vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write{};
    write.dstSet = m_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write.pImageInfo = &imageInfo;
    device.updateDescriptorSets (write, {});

    const vk::PushConstantRange pushRange (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
                                           sizeof (PushConstants));
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &*m_setLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    m_layout = vk::raii::PipelineLayout (device, pipelineLayoutInfo);

    createPipeline (context, renderPass, false);
    createPipeline (context, renderPass, true);
}

void WorldRenderer::beginFrame (const glm::mat4& viewProj, const glm::vec3& sunDir, float ambient,
                                const core::Frustum& frustum, bool wireframe) {
    m_push.viewProj = viewProj;
    m_push.sunDir = sunDir;
    m_push.ambient = ambient;
    m_frustum = frustum;
    m_wireframe = wireframe;
}

void WorldRenderer::addDraw (DrawRecord record) {
    m_byChunk[record.coord].push_back (m_draws.size ());
    m_draws.push_back (record);
}

void WorldRenderer::removeDraws (world::ChunkCoord coord) {
    if (!m_byChunk.erase (coord)) return;
    // Compact and rebuild the index: obviously correct, and removals are
    // rare (chunk remesh / unload) compared to draws.
    std::vector<DrawRecord> kept;
    kept.reserve (m_draws.size ());
    for (const auto& draw : m_draws) {
        if (draw.coord == coord) continue;
        kept.push_back (draw);
    }
    m_draws = std::move (kept);
    m_byChunk.clear ();
    for (size_t i = 0; i < m_draws.size (); ++i) {
        m_byChunk[m_draws[i].coord].push_back (i);
    }
}

void WorldRenderer::render (FrameContext& frame) {
    auto& cmd = frame.cmd;
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, m_wireframe ? m_wirePipeline : m_pipeline);
    cmd.setViewport (0, vk::Viewport (0.0f, 0.0f, static_cast<float> (frame.extent.width),
                                      static_cast<float> (frame.extent.height), 0.0f, 1.0f));
    cmd.setScissor (0, vk::Rect2D ({0, 0}, frame.extent));
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, m_layout, 0, vk::DescriptorSet (m_set), {});
    cmd.bindVertexBuffers (0, m_arena->buffer (), vk::DeviceSize (0));
    cmd.bindIndexBuffer (m_arena->buffer (), 0, vk::IndexType::eUint16);
    cmd.pushConstants (m_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
                       sizeof (PushConstants), &m_push);

    for (const auto& draw : m_draws) {
        if (!core::intersects (m_frustum, draw.bounds)) continue;
        m_push.chunkOrigin = glm::vec3 (draw.coord.x * m_dims.width, 0, draw.coord.z * m_dims.depth);
        cmd.pushConstants (m_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                           offsetof (PushConstants, chunkOrigin), sizeof (glm::vec3), &m_push.chunkOrigin);
        cmd.drawIndexed (draw.indexCount, 1, static_cast<uint32_t> (draw.indexOffset / sizeof (uint16_t)),
                         static_cast<int32_t> (draw.vertexOffset / sizeof (uint64_t)), 0);
    }
}

}  // namespace ih::render
