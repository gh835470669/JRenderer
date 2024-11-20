#pragma once

#include <vulkan/vulkan.h>
#include <imgui/imgui.h>
#include "jrenderer/window.h"
#include "main_loop_context.h"
#include "jrenderer/graphics.h"
#include "jrenderer/render_set.h"

namespace jre
{
	namespace imgui
	{
		class ImguiContext : public IWindowMessageHandler, public IRenderSetRenderer
		{
		private:
			bool m_has_new_frame = false;
			std::unique_ptr<DescriptorPool> m_descriptor_pool;

			std::shared_ptr<VertexShader> ui_vert_shader;
			std::shared_ptr<FragmentShader> ui_frag_shader;

			std::shared_ptr<const Texture2D> m_font_texture;

			std::vector<std::shared_ptr<HostVertexBuffer<ImDrawVert>>> m_vertex_buffers;
			std::vector<std::shared_ptr<HostIndexBuffer<ImDrawIdx>>> m_index_buffers;

			const Graphics &m_graphics;
			gsl::not_null<const LogicalDevice *> m_device;

			std::shared_ptr<GraphicsPipeline> m_graphics_pipeline;
			std::shared_ptr<DescriptorSet> m_descriptor_set;

			// UI params are set via push constants
			struct PushConstBlock
			{
				glm::vec2 scale;
				glm::vec2 translate;
			} push_const_block;

			static PipelineVertexInputState get_vertex_input_state();

			static void SetupImGuiStyle();

		public:
			ImguiContext(const Window &window, Graphics &graphics);
			~ImguiContext();

			void new_frame();
			void pre_draw();
			void draw(const Graphics &graphics, const CommandBuffer &command_buffer) override;
			void update_buffers();
			LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) override;

			void on_msaa_changed();
		};
	}
}