#include "jrenderer/pipeline.h"

namespace jre
{
    vk::PipelineColorBlendAttachmentState PipelineBuilder::ColorBlendAttachment::alpha()
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

    vk::PipelineColorBlendAttachmentState PipelineBuilder::ColorBlendAttachment::overwrite()
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
    vk::SharedPipeline PipelineBuilder::build()
    {
        std::vector<vk::SpecializationInfo> specs = specialization_constants | std::views::transform([](std::pair<jre::bytes, std::vector<vk::SpecializationMapEntry>> &p)
                                                                                                     {
                                                                                       const auto &[sp_constansts, sp_constants_entires] = p;
                                                                                       vk::SpecializationInfo specialization_info{};
                                                                                       specialization_info.mapEntryCount = static_cast<uint32_t>(sp_constants_entires.size());
                                                                                       specialization_info.pMapEntries = sp_constants_entires.data();
                                                                                       specialization_info.dataSize = static_cast<uint32_t>(sp_constansts.size());
                                                                                       specialization_info.pData = sp_constansts.data();
                                                                                       return specialization_info; }) |
                                                    std::ranges::to<std::vector>();
        for (const auto &[stage, spec, entry] : std::views::zip(stages, specs, shader_stage_entries))
        {
            stage.setPSpecializationInfo(&spec);
            stage.setPName(entry.c_str());
        }
        vk::PipelineViewportStateCreateInfo viewport_state;
        viewport_state
            .setPScissors(scissors.data())
            .setScissorCount(static_cast<uint32_t>(scissors.size()))
            .setPViewports(viewports.data())
            .setViewportCount(static_cast<uint32_t>(viewports.size()));
        vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_states);
        vk::PipelineVertexInputStateCreateInfo vertex_input_state(
            {},
            vertex_binding_descriptions,
            vertex_attribute_descriptions);
        color_blend.setAttachments(color_blend_attachments);
        pipeline_info
            .setPVertexInputState(&vertex_input_state)
            .setRenderPass(render_pass)
            .setPDepthStencilState(&depth_stencil)
            .setPMultisampleState(&multisampling)
            .setPRasterizationState(&rasterizer)
            .setPColorBlendState(&color_blend)
            .setPViewportState(&viewport_state)
            .setPDynamicState(&dynamic_state)
            .setPInputAssemblyState(&input_assembly)
            .setLayout(pipeline_layout)
            .setPStages(stages.data())
            .setStageCount(static_cast<uint32_t>(stages.size()));
        auto res_value = device->createGraphicsPipeline(cache, pipeline_info);
        if (res_value.result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        return vk::SharedPipeline{res_value.value, device};
    }
}