#define NOMINMAX
#include "imgui_context.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jre
{
    namespace imgui
    {
        ImguiContext::ImguiContext(VulkanPipeline &pipeline, Window &window)
        {
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

            // Setup Dear ImGui style
            // ImGui::StyleColorsDark();
            // Color scheme
            ImGuiStyle vulkanStyle = ImGui::GetStyle();
            vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
            vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
            vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
            vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
            vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

            ImGuiStyle &style = ImGui::GetStyle();
            // style = vulkanStyle;
            ImGui::StyleColorsDark();

            // Dimensions
            auto size = window.size();
            io.DisplaySize = ImVec2(std::get<0>(size), std::get<1>(size));
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
            VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
            VkDescriptorPoolSize pool_sizes[] =
                {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
                };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            vkCreateDescriptorPool(pipeline.device(), &pool_info, nullptr, &g_DescriptorPool);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = pipeline.instance();
            init_info.PhysicalDevice = pipeline.physical_device();
            init_info.Device = pipeline.device();
            init_info.QueueFamily = pipeline.graphics_queue_family();
            init_info.Queue = pipeline.graphics_queue();
            // init_info.PipelineCache = g_PipelineCache;
            init_info.DescriptorPool = g_DescriptorPool;
            init_info.RenderPass = pipeline.render_pass();
            init_info.Subpass = 0;
            init_info.MinImageCount = 2;
            init_info.ImageCount = pipeline.swap_chain_images().size();
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            // init_info.Allocator = g_Allocator;
            // init_info.CheckVkResultFn = check_vk_result;
            ImGui_ImplWin32_Init(window.hwnd());
            ImGui_ImplVulkan_Init(&init_info);
        };

        ImguiContext::~ImguiContext()
        {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        void ImguiContext::Tick(const TickContext &context)
        {
            // Start the Dear ImGui frame
            ImGui_ImplWin32_NewFrame();
            ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();
            for (auto &window : imgui_windows)
            {
                window.get().Tick();
            }

            // Render to generate draw buffers
            ImGui::Render();
        }

        void ImguiContext::Draw(const DrawContext &context)
        {
            ImDrawData *draw_data = ImGui::GetDrawData();
            const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized)
            {
                ImGui_ImplVulkan_RenderDrawData(draw_data, context.command_buffer);
            }
        }

        LRESULT ImguiContext::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
                return true;
            return 0;
        }

    }
}