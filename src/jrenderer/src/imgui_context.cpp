#define NOMINMAX
#include <vulkan/vulkan.hpp>
#include <imgui/backends/imgui_impl_win32.h>
#include "details/imgui_context.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jre
{
    namespace imgui
    {
        ImguiContext::ImguiContext(const Window &window, Graphics &graphics) : m_graphics(graphics), m_device(graphics.logical_device())
        {
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

            // Setup Dear ImGui style
            // ImGui::StyleColorsDark();
            ImguiContext::SetupImGuiStyle();

            // Dimensions
            auto size = window.size();
            io.DisplaySize = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
            io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

            // io.KeyMap 是传统的绑定法，不能与addEventKey (ImGui_ImplWin32_WndProcHandler) 一起用
            // #if defined(_WIN32)
            //             // If we directly work with os specific key codes, we need to map special key types like tab
            //             io.KeyMap[ImGuiKey_Tab] = VK_TAB;
            //             io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
            //             io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
            //             io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
            //             io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
            //             io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
            //             io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
            //             io.KeyMap[ImGuiKey_Space] = VK_SPACE;
            //             io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
            // #endif

            vk::Queue graphics_queue = graphics.logical_device()->graphics_queue();
            ui_vert_shader = std::make_shared<VertexShader>(graphics.logical_device(), "res/shaders/glsl/imgui/ui.vert.spv");
            ui_frag_shader = std::make_shared<FragmentShader>(graphics.logical_device(), "res/shaders/glsl/imgui/ui.frag.spv");

            // Create font texture

            unsigned char *fontData;
            int texWidth, texHeight;
            io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
            ImageDataRef font_image_data(texWidth, texHeight,
                                         4,
                                         fontData);

            vk::PhysicalDeviceProperties properties = graphics.physical_device()->properties();

            const ImageCreateInfo image_create_info{
                vk::ImageCreateInfo(
                    vk::ImageCreateFlags(),
                    vk::ImageType::e2D,
                    vk::Format::eR8G8B8A8Unorm,
                    vk::Extent3D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
                    1,
                    1,
                    vk::SampleCountFlagBits::e1,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                    vk::SharingMode::eExclusive),
                vk::ImageSubresourceRange(
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1),
                vk::SamplerCreateInfo(
                    vk::SamplerCreateFlags(),
                    vk::Filter::eLinear,
                    vk::Filter::eLinear,
                    vk::SamplerMipmapMode::eLinear,
                    vk::SamplerAddressMode::eClampToEdge,
                    vk::SamplerAddressMode::eClampToEdge,
                    vk::SamplerAddressMode::eClampToEdge,
                    0.0f,
                    VK_FALSE,
                    1,
                    VK_FALSE,
                    vk::CompareOp::eNever,
                    0.0f,
                    0.0f,
                    vk::BorderColor::eFloatOpaqueWhite,
                    {})};
            std::unique_ptr<const CommandBuffer> command_buffer = graphics.command_pool()->allocate_command_buffer();
            m_font_texture = std::make_shared<const Texture2D>(graphics.logical_device(),
                                                               *command_buffer.get(),
                                                               font_image_data,
                                                               &image_create_info);

            // VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
            // VkDescriptorPoolSize pool_sizes[] =
            //     {
            //         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
            //     };
            // VkDescriptorPoolCreateInfo pool_info = {};
            // pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            // pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            // pool_info.maxSets = 1;
            // pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
            // pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            // pool_info.pPoolSizes = pool_sizes;
            m_descriptor_pool = std::make_unique<DescriptorPool>(graphics.logical_device(), DescriptorPoolCreateInfo{1, 1});
            // vkCreateDescriptorPool(pipeline.device(), &pool_info, nullptr, &g_DescriptorPool);
            m_descriptor_set = graphics.create_descriptor_set({{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}});
            m_descriptor_set->update_descriptor_sets({m_font_texture.get()});

            // Store our identifier
            io.Fonts->SetTexID((ImTextureID)m_descriptor_set->descriptor_set());

            vk::PushConstantRange push_constant_range{vk::ShaderStageFlagBits::eVertex, 0, sizeof(ImDrawVert)};

            // Create pipeline layout
            m_graphics_pipeline = std::make_shared<GraphicsPipeline>(
                graphics.logical_device(),
                GraphicsPipelineCreateInfo{
                    *graphics.render_pass(),
                    ui_vert_shader,
                    ui_frag_shader,
                    get_vertex_input_state(),
                    {m_descriptor_set->descriptor_set_layout()},
                    {push_constant_range},
                    false,
                    true,
                    graphics.settings().msaa});

            // ImGui_ImplVulkan_InitInfo init_info = {};
            // init_info.Instance = graphics.instance()->instance();
            // init_info.PhysicalDevice = graphics.logical_device()->physical_device()->physical_device();
            // init_info.Device = graphics.logical_device()->device();
            // init_info.QueueFamily = graphics.logical_device()->graphics_queue_family();
            // init_info.Queue = graphics.logical_device()->graphics_queue();
            // // init_info.PipelineCache = g_PipelineCache;
            // init_info.DescriptorPool = m_descriptor_pool->descriptor_pool();
            // init_info.RenderPass = graphics.render_pass()->render_pass();
            // init_info.Subpass = 0;
            // init_info.MinImageCount = 2;
            // init_info.ImageCount = static_cast<uint32_t>(graphics.swap_chain()->images().size());
            // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            // // init_info.Allocator = g_Allocator;
            // // init_info.CheckVkResultFn = check_vk_result;
            ImGui_ImplWin32_Init(window.hwnd());
            // ImGui_ImplVulkan_Init(&init_info);
        };

        void ImguiContext::update_buffers()
        {
            ImDrawData *imDrawData = ImGui::GetDrawData();

            // Note: Alignment is done inside buffer creation
            VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
            VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

            if ((vertexBufferSize == 0) || (indexBufferSize == 0))
            {
                return;
            }

            // Upload data
            std::vector<ImDrawVert> vertices;
            std::vector<ImDrawIdx> indices;
            for (int n = 0; n < imDrawData->CmdListsCount; n++)
            {
                const ImDrawList *cmd_list = imDrawData->CmdLists[n];
                vertices.insert(vertices.end(), cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size);
                indices.insert(indices.end(), cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size);
            }

            // Update buffers only if vertex or index count has been changed compared to current buffer size

            // Vertex buffer
            if ((!m_vertex_buffer) || (m_vertex_buffer->count() < imDrawData->TotalVtxCount))
            {
                m_vertex_buffer = std::make_shared<HostVertexBuffer<ImDrawVert>>(m_device, vertices);
            }
            else
            {
                m_vertex_buffer->update_buffer(vertices);
            }

            // Index buffer
            if ((!m_index_buffer) || (m_index_buffer->count() < imDrawData->TotalIdxCount))
            {
                m_index_buffer = std::make_shared<HostIndexBuffer<ImDrawIdx>>(m_device, indices);
            }
            else
            {
                m_index_buffer->update_buffer(indices);
            }
        }

        ImguiContext::~ImguiContext()
        {
            // ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        void ImguiContext::new_frame()
        {
            // Start the Dear ImGui frame
            ImGui_ImplWin32_NewFrame();
            // ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();
            m_has_new_frame = true;
        }

        void ImguiContext::pre_draw()
        {
            if (!m_has_new_frame)
                return;
            // Render to generate draw buffers
            ImGui::Render();
            m_has_new_frame = false;
        }

        void ImguiContext::draw(const Graphics &graphics, const CommandBuffer &command_buffer)
        {

            ImDrawData *draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized)
            {
                update_buffers();

                ImGuiIO &io = ImGui::GetIO();

                command_buffer.command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline->pipeline_layout(), 0, m_descriptor_set->descriptor_set(), nullptr);
                command_buffer.command_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);

                vk::Viewport viewport = vk::Viewport{0.0f, 0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f};
                command_buffer.command_buffer().setViewport(0, viewport);

                // UI scale and translate via push constants
                push_const_block.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
                push_const_block.translate = glm::vec2(-1.0f);
                command_buffer.command_buffer().pushConstants(m_graphics_pipeline->pipeline_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock), &push_const_block);
                // vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

                // Render commands
                ImDrawData *imDrawData = ImGui::GetDrawData();
                int32_t vertexOffset = 0;
                int32_t indexOffset = 0;

                if (imDrawData->CmdListsCount > 0)
                {

                    VkDeviceSize offsets[1] = {0};
                    // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
                    command_buffer.command_buffer().bindVertexBuffers(0, 1, &m_vertex_buffer->buffer(), offsets);
                    command_buffer.command_buffer().bindIndexBuffer(m_index_buffer->buffer(), 0, vk::IndexType::eUint16);

                    for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
                    {
                        const ImDrawList *cmd_list = imDrawData->CmdLists[i];
                        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
                        {
                            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
                            vk::Rect2D scissorRect;
                            scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                            scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                            scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                            scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                            command_buffer.command_buffer().setScissor(0, scissorRect);
                            command_buffer.command_buffer().drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                            indexOffset += pcmd->ElemCount;
                        }
                        vertexOffset += cmd_list->VtxBuffer.Size;
                    }
                }
                // ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer.command_buffer());
            }
        }

        LRESULT ImguiContext::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
                return true;
            return 0;
        }

        void ImguiContext::SetupImGuiStyle()
        {
            // DUCK RED nope! is DARK RED style by for40255 from ImThemes
            ImGuiStyle &style = ImGui::GetStyle();

            style.Alpha = 1.0f;
            style.DisabledAlpha = 0.6000000238418579f;
            style.WindowPadding = ImVec2(8.0f, 8.0f);
            style.WindowRounding = 0.0f;
            style.WindowBorderSize = 1.0f;
            style.WindowMinSize = ImVec2(32.0f, 32.0f);
            style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
            style.WindowMenuButtonPosition = ImGuiDir_Left;
            style.ChildRounding = 0.0f;
            style.ChildBorderSize = 1.0f;
            style.PopupRounding = 0.0f;
            style.PopupBorderSize = 1.0f;
            style.FramePadding = ImVec2(4.0f, 3.0f);
            style.FrameRounding = 0.0f;
            style.FrameBorderSize = 0.0f;
            style.ItemSpacing = ImVec2(8.0f, 4.0f);
            style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
            style.CellPadding = ImVec2(4.0f, 2.0f);
            style.IndentSpacing = 21.0f;
            style.ColumnsMinSpacing = 6.0f;
            style.ScrollbarSize = 14.0f;
            style.ScrollbarRounding = 0.0f;
            style.GrabMinSize = 10.0f;
            style.GrabRounding = 0.0f;
            style.TabRounding = 0.0f;
            style.TabBorderSize = 0.0f;
            style.TabMinWidthForCloseButton = 0.0f;
            style.ColorButtonPosition = ImGuiDir_Right;
            style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
            style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

            style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05490196123719215f, 0.05490196123719215f, 0.05490196123719215f, 1.0f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_Border] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1176470592617989f, 0.1176470592617989f, 1.0f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.5647059082984924f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.5647059082984924f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9411764740943909f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.5647059082984924f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.501960813999176f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.8154506683349609f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8156862854957581f);
            style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.501960813999176f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.7450980544090271f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.6566523313522339f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.8039215803146362f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_Separator] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.501960813999176f);
            style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.6695278882980347f);
            style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9570815563201904f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1019607856869698f, 0.1137254908680916f, 0.1294117718935013f, 0.2000000029802322f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2039215713739395f, 0.2078431397676468f, 0.2156862765550613f, 0.2000000029802322f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3019607961177826f, 0.3019607961177826f, 0.3019607961177826f, 0.2000000029802322f);
            style.Colors[ImGuiCol_Tab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4392156898975372f);
            style.Colors[ImGuiCol_TabHovered] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TabActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.06666667014360428f, 0.06666667014360428f, 0.9725490212440491f);
            style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06666667014360428f, 0.06666667014360428f, 0.06666667014360428f, 1.0f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.9490196108818054f, 0.3450980484485626f, 0.3450980484485626f, 1.0f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.9490196108818054f, 0.3450980484485626f, 0.3450980484485626f, 1.0f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4274509847164154f, 0.3607843220233917f, 0.3607843220233917f, 1.0f);
            style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.7124463319778442f);
            style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TableBorderLight] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.196078434586525f, 0.196078434586525f, 0.196078434586525f, 0.6274510025978088f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.2588235437870026f, 0.2705882489681244f, 0.3803921639919281f, 1.0f);
            style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1803921610116959f, 0.2274509817361832f, 0.2784313857555389f, 1.0f);
            style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
            style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
            style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
        }

        PipelineVertexInputState ImguiContext::get_vertex_input_state()
        {
            // Vertex bindings an attributes based on ImGui vertex definition
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(ImDrawVert);
            binding_description.inputRate = vk::VertexInputRate::eVertex;

            return {{binding_description}, {
                                               {0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)}, // Location 0: Position
                                               {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)},  // Location 1: UV
                                               {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)} // Location 2: Color
                                           }};
        }

        void ImguiContext::set_msaa(const vk::SampleCountFlagBits &msaa)
        {
            vk::PushConstantRange push_constant_range{vk::ShaderStageFlagBits::eVertex, 0, sizeof(ImDrawVert)};
            m_graphics_pipeline = std::make_shared<GraphicsPipeline>(
                m_graphics.logical_device(),
                GraphicsPipelineCreateInfo{
                    *m_graphics.render_pass(),
                    ui_vert_shader,
                    ui_frag_shader,
                    get_vertex_input_state(),
                    {m_descriptor_set->descriptor_set_layout()},
                    {push_constant_range},
                    false,
                    true,
                    m_graphics.settings().msaa});
        }
    }
}