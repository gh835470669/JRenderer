#define NOMINMAX
#include "imgui_context.h"

namespace jre
{
    namespace imgui
    {
        struct UISettings
        {
            bool displayModels = true;
            bool displayLogos = true;
            bool displayBackground = true;
            bool animateLight = false;
            float lightSpeed = 0.25f;
            std::array<float, 50> frameTimes{};
            float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
            float lightTimer = 0.0f;
        } uiSettings;

        ImguiContext::ImguiContext(VkPhysicalDevice physical_device, VkDevice logical_device, VkCommandPool cmd_pool,
                                   VkRenderPass render_pass, VkQueue graphics_queue, Window &window, const Statistics &statistics)
            : m_window(window), device(physical_device), m_imgui_windows(statistics)
        {
            device.logicalDevice = logical_device;
            device.commandPool = cmd_pool;

            ImGui::CreateContext();

            // SRS - Set ImGui font and style scale factors to handle retina and other HiDPI displays
            ImGuiIO &io = ImGui::GetIO();
            io.FontGlobalScale = UIOverlay.scale;
            ImGuiStyle &style = ImGui::GetStyle();
            style.ScaleAllSizes(UIOverlay.scale);

            init((float)1920, (float)1080);
            initResources(render_pass, graphics_queue, "res/shaders/glsl/");
        };

        ImguiContext::~ImguiContext()
        {
            ImGui::DestroyContext();
            // Release all Vulkan resources required for rendering imGui
            vertexBuffer.destroy();
            indexBuffer.destroy();
            vkDestroyImage(device.logicalDevice, fontImage, nullptr);
            vkDestroyImageView(device.logicalDevice, fontView, nullptr);
            vkFreeMemory(device.logicalDevice, fontMemory, nullptr);
            vkDestroySampler(device.logicalDevice, sampler, nullptr);
            vkDestroyPipelineCache(device.logicalDevice, pipelineCache, nullptr);
            vkDestroyPipeline(device.logicalDevice, pipeline, nullptr);
            vkDestroyPipelineLayout(device.logicalDevice, pipelineLayout, nullptr);
            vkDestroyDescriptorPool(device.logicalDevice, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device.logicalDevice, descriptorSetLayout, nullptr);
        }

        // Initialize styles, keys, etc.
        void ImguiContext::init(float width, float height)
        {
            // Color scheme
            vulkanStyle = ImGui::GetStyle();
            vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
            vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
            vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
            vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
            vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

            setStyle(0);

            // Dimensions
            ImGuiIO &io = ImGui::GetIO();
            io.DisplaySize = ImVec2(width, height);
            io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
#if defined(_WIN32)
            // If we directly work with os specific key codes, we need to map special key types like tab
            io.KeyMap[ImGuiKey_Tab] = VK_TAB;
            io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
            io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
            io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
            io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
            io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
            io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
            io.KeyMap[ImGuiKey_Space] = VK_SPACE;
            io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
#endif
        }

        void ImguiContext::setStyle(uint32_t index)
        {
            switch (index)
            {
            case 0:
            {
                ImGuiStyle &style = ImGui::GetStyle();
                style = vulkanStyle;
                break;
            }
            case 1:
                ImGui::StyleColorsClassic();
                break;
            case 2:
                ImGui::StyleColorsDark();
                break;
            case 3:
                ImGui::StyleColorsLight();
                break;
            }
        }

        // Initialize all Vulkan resources used by the ui
        void ImguiContext::initResources(VkRenderPass renderPass, VkQueue copyQueue, const std::string &shadersPath)
        {
            ImGuiIO &io = ImGui::GetIO();

            // Create font texture
            unsigned char *fontData;
            int texWidth, texHeight;
            io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
            VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

            // SRS - Get Vulkan device driver information if available, use later for display
            if (device.extensionSupported(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
            {
                VkPhysicalDeviceProperties2 deviceProperties2 = {};
                deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                deviceProperties2.pNext = &driverProperties;
                driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
                vkGetPhysicalDeviceProperties2(device.physicalDevice, &deviceProperties2);
            }

            // Create target image for copy
            VkImageCreateInfo imageInfo = vks::initializers::imageCreateInfo();
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageInfo.extent.width = texWidth;
            imageInfo.extent.height = texHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VK_CHECK_RESULT(vkCreateImage(device.logicalDevice, &imageInfo, nullptr, &fontImage));
            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device.logicalDevice, fontImage, &memReqs);
            VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
            memAllocInfo.allocationSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = device.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VK_CHECK_RESULT(vkAllocateMemory(device.logicalDevice, &memAllocInfo, nullptr, &fontMemory));
            VK_CHECK_RESULT(vkBindImageMemory(device.logicalDevice, fontImage, fontMemory, 0));

            // Image view
            VkImageViewCreateInfo viewInfo = vks::initializers::imageViewCreateInfo();
            viewInfo.image = fontImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;
            VK_CHECK_RESULT(vkCreateImageView(device.logicalDevice, &viewInfo, nullptr, &fontView));

            // Staging buffers for font data upload
            vks::Buffer stagingBuffer;

            VK_CHECK_RESULT(device.createBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer,
                uploadSize));

            stagingBuffer.map();
            memcpy(stagingBuffer.mapped, fontData, uploadSize);
            stagingBuffer.unmap();

            // Copy buffer data to font image
            VkCommandBuffer copyCmd = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

            // Prepare for transfer
            vks::tools::setImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Copy
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = texWidth;
            bufferCopyRegion.imageExtent.height = texHeight;
            bufferCopyRegion.imageExtent.depth = 1;

            vkCmdCopyBufferToImage(
                copyCmd,
                stagingBuffer.buffer,
                fontImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

            // Prepare for shader read
            vks::tools::setImageLayout(
                copyCmd,
                fontImage,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            device.flushCommandBuffer(copyCmd, copyQueue, true);

            stagingBuffer.destroy();

            // Font texture Sampler
            VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(vkCreateSampler(device.logicalDevice, &samplerInfo, nullptr, &sampler));

            // Descriptor pool
            std::vector<VkDescriptorPoolSize> poolSizes = {
                vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
            VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
            VK_CHECK_RESULT(vkCreateDescriptorPool(device.logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

            // Descriptor set layout
            std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
            };
            VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device.logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

            // Descriptor set
            VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device.logicalDevice, &allocInfo, &descriptorSet));
            VkDescriptorImageInfo fontDescriptor = vks::initializers::descriptorImageInfo(
                sampler,
                fontView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor)};
            vkUpdateDescriptorSets(device.logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

            // Pipeline cache
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            VK_CHECK_RESULT(vkCreatePipelineCache(device.logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));

            // Pipeline layout
            // Push constants for UI rendering parameters
            VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            VK_CHECK_RESULT(vkCreatePipelineLayout(device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

            // Setup graphics pipeline for UI rendering
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
                vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

            VkPipelineRasterizationStateCreateInfo rasterizationState =
                vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

            // Enable blending
            VkPipelineColorBlendAttachmentState blendAttachmentState{};
            blendAttachmentState.blendEnable = VK_TRUE;
            blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
            blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

            VkPipelineColorBlendStateCreateInfo colorBlendState =
                vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

            VkPipelineDepthStencilStateCreateInfo depthStencilState =
                vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

            VkPipelineViewportStateCreateInfo viewportState =
                vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

            VkPipelineMultisampleStateCreateInfo multisampleState =
                vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

            std::vector<VkDynamicState> dynamicStateEnables = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicState =
                vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
            shaderStages.resize(2);

            VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);

            pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
            pipelineCreateInfo.pRasterizationState = &rasterizationState;
            pipelineCreateInfo.pColorBlendState = &colorBlendState;
            pipelineCreateInfo.pMultisampleState = &multisampleState;
            pipelineCreateInfo.pViewportState = &viewportState;
            pipelineCreateInfo.pDepthStencilState = &depthStencilState;
            pipelineCreateInfo.pDynamicState = &dynamicState;
            pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineCreateInfo.pStages = shaderStages.data();

            // Vertex bindings an attributes based on ImGui vertex definition
            std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                vks::initializers::vertexInputBindingDescription(0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX),
            };
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
                vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),  // Location 0: Position
                vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),   // Location 1: UV
                vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)), // Location 0: Color
            };
            VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
            vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
            vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
            vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
            vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

            pipelineCreateInfo.pVertexInputState = &vertexInputState;

            shaderStages[0] = loadShader(shadersPath + "imgui/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
            shaderStages[1] = loadShader(shadersPath + "imgui/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
        }

        // Starts a new imGui frame and sets up windows and ui elements
        void ImguiContext::newFrame(bool updateFrameGraph)
        {
            ImGui::NewFrame();
            m_imgui_windows.Tick();
            // Init imGui windows and elements

            // Debug window
            // ImGui::SetWindowPos(ImVec2(20 * UIOverlay.scale, 20 * UIOverlay.scale));
            // ImGui::SetWindowSize(ImVec2(300 * UIOverlay.scale, 300 * UIOverlay.scale));
            // ImGui::TextUnformatted(title.c_str());
            // ImGui::TextUnformatted(device.properties.deviceName);

            // SRS - Display Vulkan API version and device driver information if available (otherwise blank)
            // ImGui::Text("Vulkan API %i.%i.%i", VK_API_VERSION_MAJOR(device.properties.apiVersion), VK_API_VERSION_MINOR(device.properties.apiVersion), VK_API_VERSION_PATCH(device.properties.apiVersion));
            // ImGui::Text("%s %s", driverProperties.driverName, driverProperties.driverInfo);

            // Update frame time display
            // if (updateFrameGraph)
            // {
            //     std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
            //     float frameTime = 1000.0f / (frameTimer * 1000.0f);
            //     uiSettings.frameTimes.back() = frameTime;
            //     if (frameTime < uiSettings.frameTimeMin)
            //     {
            //         uiSettings.frameTimeMin = frameTime;
            //     }
            //     if (frameTime > uiSettings.frameTimeMax)
            //     {
            //         uiSettings.frameTimeMax = frameTime;
            //     }
            // }

            // ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));

            // ImGui::Text("Camera");
            // ImGui::InputFloat3("position", &camera.position.x);
            // ImGui::InputFloat3("rotation", &camera.rotation.x);

            // Example settings window
            // ImGui::SetNextWindowPos(ImVec2(20 * UIOverlay.scale, 360 * UIOverlay.scale));
            // ImGui::SetNextWindowSize(ImVec2(300 * UIOverlay.scale, 200 * UIOverlay.scale));
            // ImGui::Begin("Example settings");
            // ImGui::Checkbox("Render models", &uiSettings.displayModels);
            // ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
            // ImGui::Checkbox("Display background", &uiSettings.displayBackground);
            // ImGui::Checkbox("Animate light", &uiSettings.animateLight);
            // ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
            // // ImGui::ShowStyleSelector("UI style");

            // if (ImGui::Combo("UI style", &selectedStyle, "Vulkan\0Classic\0Dark\0Light\0"))
            // {
            //     setStyle(selectedStyle);
            // }

            // ImGui::End();

            // SRS - ShowDemoWindow() sets its own initial position and size, cannot override here
            // ImGui::ShowDemoWindow();

            // Render to generate draw buffers
            ImGui::Render();
        }

        // Update vertex and index buffer containing the imGui elements when required
        void ImguiContext::updateBuffers()
        {
            ImDrawData *imDrawData = ImGui::GetDrawData();

            // Note: Alignment is done inside buffer creation
            VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
            VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

            if ((vertexBufferSize == 0) || (indexBufferSize == 0))
            {
                return;
            }

            // Update buffers only if vertex or index count has been changed compared to current buffer size

            // Vertex buffer
            if ((vertexBuffer.buffer == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount))
            {
                vertexBuffer.unmap();
                vertexBuffer.destroy();
                VK_CHECK_RESULT(device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexBuffer, vertexBufferSize));
                vertexCount = imDrawData->TotalVtxCount;
                vertexBuffer.map();
            }

            // Index buffer
            if ((indexBuffer.buffer == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount))
            {
                indexBuffer.unmap();
                indexBuffer.destroy();
                VK_CHECK_RESULT(device.createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexBuffer, indexBufferSize));
                indexCount = imDrawData->TotalIdxCount;
                indexBuffer.map();
            }

            // Upload data
            ImDrawVert *vtxDst = (ImDrawVert *)vertexBuffer.mapped;
            ImDrawIdx *idxDst = (ImDrawIdx *)indexBuffer.mapped;

            for (int n = 0; n < imDrawData->CmdListsCount; n++)
            {
                const ImDrawList *cmd_list = imDrawData->CmdLists[n];
                memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                vtxDst += cmd_list->VtxBuffer.Size;
                idxDst += cmd_list->IdxBuffer.Size;
            }

            // Flush to make writes visible to GPU
            vertexBuffer.flush();
            indexBuffer.flush();
        }

        // Draw current imGui frame into a command buffer
        void ImguiContext::drawFrame(VkCommandBuffer commandBuffer)
        {
            ImGuiIO &io = ImGui::GetIO();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            VkViewport viewport = vks::initializers::viewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            // UI scale and translate via push constants
            pushConstBlock.scale = ImVec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
            pushConstBlock.translate = ImVec2(-1.0f, -1.0f);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

            // Render commands
            ImDrawData *imDrawData = ImGui::GetDrawData();
            int32_t vertexOffset = 0;
            int32_t indexOffset = 0;

            if (imDrawData->CmdListsCount > 0)
            {

                VkDeviceSize offsets[1] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

                for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
                {
                    const ImDrawList *cmd_list = imDrawData->CmdLists[i];
                    for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
                    {
                        const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
                        VkRect2D scissorRect;
                        scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                        scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                        scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                        scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                        vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                        indexOffset += pcmd->ElemCount;
                    }
                    vertexOffset += cmd_list->VtxBuffer.Size;
                }
            }
        }
        void ImguiContext::Tick(const TickContext &context)
        {
            // Update imGui
            ImGuiIO &io = ImGui::GetIO();

            RECT rect;
            GetWindowRect(m_window.hwnd(), &rect);   // 获取窗口大小
            int win_width = rect.right - rect.left;  // 计算窗口宽度
            int win_height = rect.bottom - rect.top; // 计算窗口高度

            io.DisplaySize = ImVec2((float)win_width, (float)win_height);
            io.DeltaTime = frameTimer;

            io.MousePos = ImVec2(mousePos.x, mousePos.y);
            io.MouseDown[0] = mouseButtons.left;
            io.MouseDown[1] = mouseButtons.right;
            io.MouseDown[2] = mouseButtons.middle;

            int frameCounter = 0;
            newFrame(frameCounter == 0);
            updateBuffers();
        }
        void ImguiContext::Draw(const DrawContext &context)
        {
            if (UIOverlay.visible)
            {
                drawFrame(context.command_buffer);
            }
        }
        LRESULT ImguiContext::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            switch (msg)
            {
            case WM_LBUTTONDOWN:
                mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
                mouseButtons.left = true;
                break;
            case WM_RBUTTONDOWN:
                mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
                mouseButtons.right = true;
                break;
            case WM_MBUTTONDOWN:
                mousePos = ImVec2((float)LOWORD(lp), (float)HIWORD(lp));
                mouseButtons.middle = true;
                break;
            case WM_LBUTTONUP:
                mouseButtons.left = false;
                break;
            case WM_RBUTTONUP:
                mouseButtons.right = false;
                break;
            case WM_MBUTTONUP:
                mouseButtons.middle = false;
                break;
            case WM_MOUSEMOVE:
            {
                handleMouseMove(LOWORD(lp), HIWORD(lp));
                break;
            }
            default:
                return 0;
            }
            return 0;
        }

        void ImguiContext::handleMouseMove(int32_t x, int32_t y)
        {
            int32_t dx = (int32_t)mousePos.x - x;
            int32_t dy = (int32_t)mousePos.y - y;

            bool handled = false;

            ImGuiIO &io = ImGui::GetIO();
            handled = io.WantCaptureMouse;
            if (handled)
            {
                mousePos = ImVec2((float)x, (float)y);
                return;
            }
            mousePos = ImVec2((float)x, (float)y);
        }
        VkPipelineShaderStageCreateInfo ImguiContext::loadShader(std::string fileName, VkShaderStageFlagBits stage)
        {
            VkPipelineShaderStageCreateInfo shaderStage = {};
            shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
            shaderStage.module = vks::tools::loadShader(fileName.c_str(), device.logicalDevice);
#endif
            shaderStage.pName = "main";
            assert(shaderStage.module != VK_NULL_HANDLE);
            shaderModules.push_back(shaderStage.module);
            return shaderStage;
        }
    }
}