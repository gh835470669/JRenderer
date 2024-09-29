#pragma once

#include "vulkan/vulkan.h"
#include "../../../../imgui/imgui.h"
#include "window.h"
#include "main_loop_context.h"
#include "vulkan_pipeline.h"
// #include "../../imgui/backends/imgui_impl_win32.h"
// #include "../../imgui/backends/imgui_impl_vulkan.h"

namespace jre
{
	namespace imgui
	{
		class ImguiContext : public IWindowMessageHandler, IDrawable
		{
		private:
			bool m_has_new_frame = false;

		public:
			ImguiContext(VulkanPipeline &pipeline, Window &window);
			~ImguiContext();

			void new_frame();
			void draw(const DrawContext &context) override;
			LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) override;
		};
	}
}