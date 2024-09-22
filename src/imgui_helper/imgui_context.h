#pragma once

#include "vulkan/vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanUIOverlay.h"
#include "imgui.h"
#include "imgui_window/imgui_window_base.h"
#include "../window.h"
#include "../main_loop_context.h"
#include "../statistics/statistics.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_vulkan.h"
#include "../vulkan_pipeline.h"
#include <array>

namespace jre
{
	namespace imgui
	{
		class ImguiContext : public IWindowMessageHandler, ITickable, IDrawable
		{
		private:
		public:
			ImguiContext(VulkanPipeline &pipeline, Window &window);
			~ImguiContext();

			std::vector<std::reference_wrapper<ImguiWindow>> imgui_windows;

			void Tick(const TickContext &context) override;
			void Draw(const DrawContext &context) override;
			LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) override;
		};
	}
}