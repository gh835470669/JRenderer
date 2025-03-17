#pragma once

#include <vulkan/vulkan.h>
#include <imgui/imgui.h>
#include "jrenderer/window.h"
#include "jrenderer/drawer/command_buffer_recordable.h"
#include "jrenderer/graphics.h"

namespace jre
{
	namespace imgui
	{
		class ScopedFrame
		{
		public:
			ScopedFrame();
			ScopedFrame(const ScopedFrame &) = delete;
			ScopedFrame &operator=(const ScopedFrame &) = delete;
			ScopedFrame(ScopedFrame &&) = delete;
			ScopedFrame &operator=(ScopedFrame &&) = delete;
			~ScopedFrame();
		};

		class ImguiDrawer : public RenderSubpassDrawable
		{
		public:
			ImguiDrawer(const Window &window, Graphics &graphics);
			ImguiDrawer(const ImguiDrawer &) = delete;
			ImguiDrawer &operator=(const ImguiDrawer &) = delete;
			ImguiDrawer(ImguiDrawer &&) = delete;
			ImguiDrawer &operator=(ImguiDrawer &&) = delete;
			~ImguiDrawer();

			LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

			void on_draw(Graphics &graphics, vk::CommandBuffer command_buffer) override;
			void on_set_msaa(Graphics &graphics) override;
			void on_resize(uint32_t width, uint32_t height);

		private:
			vk::SharedDescriptorSet m_descriptor_set;
			vk::SharedShaderModule ui_vert_shader;
			vk::SharedShaderModule ui_frag_shader;
			DeviceImage m_font_texture;
			struct CPUFrameResource
			{
				HostMesh<ImDrawVert, ImDrawIdx> mesh;
			};
			std::vector<CPUFrameResource> frames;
			vk::SharedPipelineLayout m_pipeline_layout;
			vk::SharedPipeline m_pipeline;

			// UI params are set via push constants
			struct PushConstBlock
			{
				glm::vec2 scale;
				glm::vec2 translate;
			};
			static void SetupImGuiStyle();

			void create_pipeline(Graphics &graphics);

			void update_draw_resources(Graphics &graphics);
			void update_buffers(Graphics &graphics);
			void record_command_buffer(Graphics &graphics, vk::CommandBuffer command_buffer);
		};
	}
}