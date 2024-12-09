#include "jrenderer/graphics_pipeline.h"
#include "jrenderer/logical_device.h"
#include "jrenderer/shader.h"
#include <gsl/assert>

namespace jre
{
    GraphicsPipeline::GraphicsPipeline(gsl::not_null<const LogicalDevice *> device, const GraphicsPipelineCreateInfo &create_info) : m_device(device)
    {
        Expects(create_info.vertex_shader != nullptr && create_info.fragment_shader != nullptr);

        const auto &[sp_constansts, sp_constants_entires] = convert_to<std::pair<jre::bytes, std::vector<vk::SpecializationMapEntry>>>(create_info.specialization_constants);
        vk::SpecializationInfo specialization_info{};
        specialization_info.mapEntryCount = static_cast<uint32_t>(sp_constants_entires.size());
        specialization_info.pMapEntries = sp_constants_entires.data();
        specialization_info.dataSize = static_cast<uint32_t>(sp_constansts.size());
        specialization_info.pData = sp_constansts.data();

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{
            create_info.vertex_shader->shader_stage_info(),
            create_info.fragment_shader->shader_stage_info()};
        for (auto &stage : shader_stages)
        {
            stage.pSpecializationInfo = &specialization_info;
        }

        // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        // baked into an immutable pipeline state object
        // as a static part of the pipeline or as a dynamic state set in the command buffer.
        // When opting for dynamic viewport(s) and scissor rectangle(s) you need to enable the respective dynamic states for the pipeline:
        std::array<vk::DynamicState, 2> dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor};

        vk::PipelineViewportStateCreateInfo viewport_state;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        //  Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the VkPipelineViewportStateCreateInfo struct.
        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        vk::PipelineInputAssemblyStateCreateInfo input_assembly;
        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        // VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
        input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
        input_assembly.primitiveRestartEnable = vk::False; // 不懂

        // The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader.
        // It also performs 【depth testing】, 【face culling】 and 【the scissor test】,
        // and it can be configured to output fragments that fill entire polygons or just the edges (【wireframe】 rendering).
        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them. This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature.
        rasterizer.depthClampEnable = vk::False;
        // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage. This basically【[disables any output to the framebuffer】.
        rasterizer.rasterizerDiscardEnable = vk::False;
        // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
        // VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
        // VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        // The lineWidth member is straightforward, it describes the thickness of lines in terms of number of fragments. The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eNone;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

        // The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's slope. This is sometimes used for shadow mapping,
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

        // GPU multisampling： disable
        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = create_info.msaa;
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional

        // Color blending
        // After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending and there are two ways to do it:
        // 1.  Mix the old and new value to produce a final color
        // 2.  Combine the old and new value using a bitwise operation

        vk::PipelineColorBlendAttachmentState color_blend_attachment{};
        try
        {
            bool alpha_blending_enable = std::get<bool>(create_info.alpha_blending);
            color_blend_attachment = alpha_blending_enable ? GraphicsPipeline::defulat_color_blend_attachment_alpha() : GraphicsPipeline::defulat_color_blend_attachment_overwrite();
        }
        catch (const std::bad_variant_access &)
        {
            color_blend_attachment = std::get<vk::PipelineColorBlendAttachmentState>(create_info.alpha_blending);
        }

        // The second structure references the array of structures for all of the framebuffers and allows you to set blend constants that you can use as blend factors in the aforementioned calculations.
        vk::PipelineColorBlendStateCreateInfo color_blending{};
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = vk::LogicOp::eCopy; // Optional
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f; // Optional
        color_blending.blendConstants[1] = 0.0f; // Optional
        color_blending.blendConstants[2] = 0.0f; // Optional
        color_blending.blendConstants[3] = 0.0f; // Optional

        // 【depth testing】
        vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.depthTestEnable = create_info.enable_depth_stencil ? VK_TRUE : VK_FALSE;
        depth_stencil.depthWriteEnable = create_info.enable_depth_stencil ? VK_TRUE : VK_FALSE;                         // Optional
        depth_stencil.depthCompareOp = create_info.enable_depth_stencil ? vk::CompareOp::eLess : vk::CompareOp::eNever; // Optional
        depth_stencil.depthBoundsTestEnable = VK_FALSE;                                                                 // Optional
        depth_stencil.minDepthBounds = 0.0f;                                                                            // Optional
        depth_stencil.maxDepthBounds = 1.0f;                                                                            // Optional
        depth_stencil.stencilTestEnable = VK_FALSE;                                                                     // Optional
        depth_stencil.front = vk::StencilOpState();                                                                     // Optional
        depth_stencil.back = vk::StencilOpState();                                                                      // Optional

        vk::PipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(create_info.descriptor_set_layouts.size());
        pipeline_layout_info.pSetLayouts = create_info.descriptor_set_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(create_info.push_constant_ranges.size());
        pipeline_layout_info.pPushConstantRanges = create_info.push_constant_ranges.data();
        m_pipeline_layout = m_device->device().createPipelineLayout(pipeline_layout_info);

        vk::PipelineVertexInputStateCreateInfo vertex_input_state = create_info.vertex_input_info.pipeline_vertex_input_state();

        // Pipeline State Objects (PSO) in Vulkan
        vk::GraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = m_pipeline_layout;
        pipeline_info.renderPass = create_info.render_pass;

        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = nullptr; // create a new graphics pipeline by deriving from an existing pipeline
        pipeline_info.basePipelineIndex = -1;
        auto res_value = m_device->device().createGraphicsPipeline(nullptr, pipeline_info);
        if (res_value.result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        m_pipeline = res_value.value;
    }

    GraphicsPipeline::GraphicsPipeline(gsl::not_null<const LogicalDevice *> device, const vk::GraphicsPipelineCreateInfo &create_info) : m_device(device)
    {
        m_pipeline = m_device->device().createGraphicsPipeline(nullptr, create_info).value;
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        m_device->device().destroyPipelineLayout(m_pipeline_layout);
        m_device->device().destroyPipeline(m_pipeline);
    }
}

vk::PipelineColorBlendAttachmentState jre::GraphicsPipeline::defulat_color_blend_attachment_alpha()
{
    // Color blending
    // After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending and there are two ways to do it:
    // 1.  Mix the old and new value to produce a final color
    // 2.  Combine the old and new value using a bitwise operation
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return color_blend_attachment;

    // 操作伪代码
    // if (blendEnable) {
    // finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    // finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
    // } else {
    //    finalColor = newColor;
    //}
    // finalColor = finalColor & colorWriteMask;
}

vk::PipelineColorBlendAttachmentState jre::GraphicsPipeline::defulat_color_blend_attachment_overwrite()
{
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
    return color_blend_attachment;
}
