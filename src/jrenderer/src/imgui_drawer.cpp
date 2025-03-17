#include <vulkan/vulkan.hpp>
#include <imgui/backends/imgui_impl_win32.h>
#include "jrenderer/drawer/imgui_drawer.h"
#include "jrenderer/concrete_uniform_buffers.h"
#include "jrenderer/mesh_drawer.h"
#include "jrenderer/descriptor_update.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jre
{
    namespace imgui
    {
        ImguiDrawer::ImguiDrawer(const Window &window, Graphics &graphics)
            : m_descriptor_set(),
              ui_vert_shader(),
              ui_frag_shader(),
              m_font_texture(),
              m_pipeline_layout(),
              m_pipeline()
        {
            vk::SharedDevice device = graphics.logical_device();
            vk::PhysicalDevice physical_device = graphics.physical_device();
            vk::Queue transfer_queue = graphics.transfer_queue().get();

            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

            // Setup Dear ImGui style
            // ImGui::StyleColorsDark();
            ImguiDrawer::SetupImGuiStyle();

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

            auto [descriptor_pool, descriptor_set_layout] = vk::shared::make_descriptor_pool_with_layout(
                device,
                1,
                {{{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}}});
            m_descriptor_set = vk::shared::allocate_one_descriptor_set(
                descriptor_pool,
                descriptor_set_layout.get());

            // shaders
            ui_vert_shader = vk::shared::create_shader_from_spv_file(device, "res/shaders/glsl/imgui/ui.vert.spv");
            ui_frag_shader = vk::shared::create_shader_from_spv_file(device, "res/shaders/glsl/imgui/ui.frag.spv");

            // Create font texture
            unsigned char *fontData;
            int tex_width, tex_height;
            io.Fonts->GetTexDataAsRGBA32(&fontData, &tex_width, &tex_height);
            m_font_texture = TextureBuilder(device,
                                            physical_device,
                                            vk::shared::allocate_one_command_buffer(graphics.transfer_command_pool()).get(),
                                            transfer_queue,
                                            TextureData{
                                                static_cast<uint32_t>(tex_width),
                                                static_cast<uint32_t>(tex_height),
                                                4,
                                                fontData})
                                 .set_generate_mipmaps(false)
                                 .set_sampler(make_sampler_create_info())
                                 .build();
            // update descriptor set
            DescripterSetUpdater{
                device.get()}
                .write_combined_image_sampler(m_font_texture.sampler.get(), m_font_texture.image_view.get())
                .update(m_descriptor_set.get());

            // Store our identifier
            io.Fonts->SetTexID((ImTextureID)m_descriptor_set.get());

            // Create pipeline layout
            m_pipeline_layout = vk::shared::create_pipeline_layout(
                device,
                {descriptor_set_layout.get()},
                {{vk::ShaderStageFlagBits::eVertex, 0, sizeof(ImDrawVert)}});
            // Create pipeline
            create_pipeline(graphics);

            frames.resize(graphics.cpu_frames().size());

            ImGui_ImplWin32_Init(window.hwnd());
        };

        void ImguiDrawer::create_pipeline(Graphics &graphics)
        {
            m_pipeline = PipelineBuilder{
                graphics.logical_device(),
                m_pipeline_layout.get(),
                graphics.render_pass().get()}
                             .add_vertex_shader(ui_vert_shader.get())
                             .add_fragment_shader(ui_frag_shader.get())
                             .add_vertex_input_binding(vk::VertexInputBindingDescription{0, sizeof(ImDrawVert), vk::VertexInputRate::eVertex})
                             .add_vertex_input_attributes({{0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos)}, {1, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv)}, {2, 0, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col)}})
                             .add_dynamic_state(vk::DynamicState::eViewport)
                             .add_dynamic_state(vk::DynamicState::eScissor)
                             .enable_depth(false)
                             .add_color_blend_attachment(PipelineBuilder::ColorBlendAttachment::alpha())
                             .set_multisampling(graphics.settings().msaa)
                             .set_rasterizer(vk::PolygonMode::eFill, 1.0f, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
                             .build();
        }

        ImguiDrawer::~ImguiDrawer()
        {
            // ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        void ImguiDrawer::update_draw_resources(Graphics &graphics)
        {
            update_buffers(graphics);
        }

        void ImguiDrawer::update_buffers(Graphics &graphics)
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
            CPUFrameResource &frame = frames[graphics.current_cpu_frame()];
            // Vertex buffer
            if (frame.mesh.vertex_buffer.count() < static_cast<uint32_t>(imDrawData->TotalVtxCount))
            {
                frame.mesh.vertex_buffer = HostArrayBufferBuilder<ImDrawVert>(graphics.logical_device(),
                                                                              graphics.physical_device(),
                                                                              vertices)
                                               .set_usage(vk::BufferUsageFlagBits::eVertexBuffer)
                                               .build();
            }
            else
            {
                frame.mesh.vertex_buffer.update(vertices);
            }

            // Index buffer
            if (frame.mesh.index_buffer.count() < static_cast<uint32_t>(imDrawData->TotalIdxCount))
            {
                frame.mesh.index_buffer = HostArrayBufferBuilder<ImDrawIdx>(graphics.logical_device(),
                                                                            graphics.physical_device(),
                                                                            indices)
                                              .set_usage(vk::BufferUsageFlagBits::eIndexBuffer)
                                              .build();
            }
            else
            {
                frame.mesh.index_buffer.update(indices);
            }
        }
        void ImguiDrawer::record_command_buffer(Graphics &graphics, vk::CommandBuffer command_buffer)
        {

            ImGuiIO &io = ImGui::GetIO();

            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout.get(), 0, m_descriptor_set.get(), nullptr);
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
            // UI scale and translate via push constants
            PushConstBlock push_const_block;
            push_const_block.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
            push_const_block.translate = glm::vec2(-1.0f);
            command_buffer.pushConstants(m_pipeline_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstBlock), &push_const_block);

            vk::Viewport viewport = vk::Viewport{0.0f, 0.0f, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f};
            command_buffer.setViewport(0, viewport);

            // Render commands
            CPUFrameResource &frame = frames[graphics.current_cpu_frame()];
            frame.mesh.sub_meshes.clear();
            ImDrawData *imDrawData = ImGui::GetDrawData();
            if (imDrawData->CmdListsCount > 0)
            {
                std::vector<vk::Rect2D> scissors;
                scissors.reserve(imDrawData->CmdListsCount);
                int32_t vertex_offset = 0;
                int32_t index_offset = 0;
                for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
                {
                    const ImDrawList *cmd_list = imDrawData->CmdLists[i];
                    for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
                    {
                        const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
                        vk::Rect2D scissor_rect;
                        scissor_rect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                        scissor_rect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                        scissor_rect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                        scissor_rect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                        scissors.push_back(scissor_rect);
                        frame.mesh.sub_meshes.push_back(SubMesh(
                            vertex_offset,
                            index_offset,
                            pcmd->ElemCount));
                        index_offset += pcmd->ElemCount;
                    }
                    vertex_offset += cmd_list->VtxBuffer.Size;
                }

                draw_mesh(&frame.mesh, command_buffer, scissors);
            }
        }

        void ImguiDrawer::on_draw(Graphics &graphics, vk::CommandBuffer command_buffer)
        {
            update_draw_resources(graphics);
            record_command_buffer(graphics, command_buffer);
        }

        LRESULT ImguiDrawer::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
                return true;
            return 0;
        }

        void ImguiDrawer::SetupImGuiStyle()
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

        void ImguiDrawer::on_set_msaa(Graphics &graphics)
        {
            create_pipeline(graphics);
        }

        void ImguiDrawer::on_resize(uint32_t width, uint32_t height)
        {
            ImGuiIO &io = ImGui::GetIO();
            io.DisplaySize = ImVec2(width, height);
        }
    }
}

jre::imgui::ScopedFrame::ScopedFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

jre::imgui::ScopedFrame::~ScopedFrame()
{
    // Render to generate draw buffers
    ImGui::Render();
}
