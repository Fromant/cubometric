#include "render/renderers/triangle_renderer.h"

#include <array>
#include <cstddef>

#include "render/vulkan/shader.h"

namespace ih::render {

namespace {

struct Vertex {
    float pos[2];
    float color[3];
};

constexpr std::array<Vertex, 3> TRIANGLE = {{
    {{-0.6f, -0.5f}, {1.0f, 0.2f, 0.2f}},
    {{0.6f, -0.5f}, {0.2f, 1.0f, 0.2f}},
    {{0.0f, 0.6f}, {0.3f, 0.4f, 1.0f}},
}};

}  // namespace

void TriangleRenderer::init (vulkan::Context& context, vk::RenderPass renderPass) {
    auto& device = context.device ();

    m_vertexBuffer.create (context, sizeof (TRIANGLE), vk::BufferUsageFlagBits::eVertexBuffer, TRIANGLE.data ());

    const auto vertModule = vulkan::loadShaderModule (device, "shaders/triangle.vert.spv");
    const auto fragModule = vulkan::loadShaderModule (device, "shaders/triangle.frag.spv");
    const std::array stages = {
        vk::PipelineShaderStageCreateInfo ({}, vk::ShaderStageFlagBits::eVertex, vertModule, "main"),
        vk::PipelineShaderStageCreateInfo ({}, vk::ShaderStageFlagBits::eFragment, fragModule, "main"),
    };

    const vk::VertexInputBindingDescription binding (0, sizeof (Vertex), vk::VertexInputRate::eVertex);
    const std::array attributes = {
        vk::VertexInputAttributeDescription (0, 0, vk::Format::eR32G32Sfloat, offsetof (Vertex, pos)),
        vk::VertexInputAttributeDescription (1, 0, vk::Format::eR32G32B32Sfloat, offsetof (Vertex, color)),
    };
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t> (attributes.size ());
    vertexInput.pVertexAttributeDescriptions = attributes.data ();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
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

    m_layout = vk::raii::PipelineLayout (device, vk::PipelineLayoutCreateInfo{});

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

    m_pipeline = vk::raii::Pipeline (device, nullptr, pipelineInfo);
}

void TriangleRenderer::render (FrameContext& frame) {
    auto& cmd = frame.cmd;
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, m_pipeline);
    cmd.setViewport (0, vk::Viewport (0.0f, 0.0f, static_cast<float> (frame.extent.width),
                                      static_cast<float> (frame.extent.height), 0.0f, 1.0f));
    cmd.setScissor (0, vk::Rect2D ({0, 0}, frame.extent));
    cmd.bindVertexBuffers (0, m_vertexBuffer.handle (), vk::DeviceSize (0));
    cmd.draw (3, 1, 0, 0);
}

}  // namespace ih::render
