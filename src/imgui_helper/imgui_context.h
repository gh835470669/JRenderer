#pragma once

#include "vulkan/vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanUIOverlay.h"
#include "imgui.h"
#include "imgui_window/imgui_windows.h"
#include "../window.h"
#include "../main_loop_context.h"
#include "../statistics/statistics.h"
#include <array>

namespace jre
{
	namespace imgui
	{
		class ImguiContext : public IWindowMessageHandler
		{
			friend class ImWindows;

		private:
			// Vulkan resources for rendering the UI
			VkSampler sampler;
			vks::Buffer vertexBuffer;
			vks::Buffer indexBuffer;
			int32_t vertexCount = 0;
			int32_t indexCount = 0;
			VkDeviceMemory fontMemory = VK_NULL_HANDLE;
			VkImage fontImage = VK_NULL_HANDLE;
			VkImageView fontView = VK_NULL_HANDLE;
			VkPipelineCache pipelineCache;
			VkPipelineLayout pipelineLayout;
			VkPipeline pipeline;
			VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorSet descriptorSet;
			vks::VulkanDevice device;
			vks::UIOverlay UIOverlay;
			std::vector<VkShaderModule> shaderModules;
			std::string title = "Statistic";
			/** @brief Last frame time measured using a high performance timer (if available) */
			float frameTimer = 1.0f;

			// Camera camera;

			VkPhysicalDeviceDriverProperties driverProperties = {};

			ImGuiStyle vulkanStyle;
			int selectedStyle = 0;

			Window &m_window;
			ImWindows m_imgui_windows;

			ImVec2 mousePos;
			struct MouseButton
			{
				bool left = false;
				bool right = false;
				bool middle = false;
			};
			MouseButton mouseButtons;

		public:
			// UI params are set via push constants
			struct PushConstBlock
			{
				ImVec2 scale;
				ImVec2 translate;
			} pushConstBlock;

			ImguiContext(VkPhysicalDevice physical_device, VkDevice logical_device, VkCommandPool cmd_pool,
						 VkRenderPass render_pass, VkQueue graphics_queue, Window &window, const Statistics &statistics);

			~ImguiContext();

			// Initialize styles, keys, etc.
			void init(float width, float height);

			void setStyle(uint32_t index);

			// Initialize all Vulkan resources used by the ui
			void initResources(VkRenderPass renderPass, VkQueue copyQueue, const std::string &shadersPath);

			// Starts a new imGui frame and sets up windows and ui elements
			void newFrame(bool updateFrameGraph);

			// Update vertex and index buffer containing the imGui elements when required
			void updateBuffers();
			// Draw current imGui frame into a command buffer
			void drawFrame(VkCommandBuffer commandBuffer);

			VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

			void Tick(const TickContext &context);
			void Draw(const DrawContext &context);

			LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
			void handleMouseMove(int32_t x, int32_t y);
		};
	}
}