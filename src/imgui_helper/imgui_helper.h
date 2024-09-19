#pragma once

#include "vulkan/vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanExampleBase.h"
#include "imgui.h"
#include <array>

class ImguiHelper
{

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
	vks::VulkanDevice *device;
    VulkanExampleBase *example;
	VkPhysicalDeviceDriverProperties driverProperties = {};
	ImGuiStyle vulkanStyle;
	int selectedStyle = 0;


public:
	static ImVec2 mousePos;
	struct MouseButton {
		bool left = false;
		bool right = false;
		bool middle = false;
	};
	static MouseButton mouseButtons;

	// UI params are set via push constants
	struct PushConstBlock {
		ImVec2 scale;
		ImVec2 translate;
	} pushConstBlock;

	ImguiHelper(VulkanExampleBase *example=nullptr);

	~ImguiHelper();

	// Initialize styles, keys, etc.
	void init(float width, float height);

	void setStyle(uint32_t index);

	// Initialize all Vulkan resources used by the ui
	void initResources(VkRenderPass renderPass, VkQueue copyQueue, const std::string& shadersPath);

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame(VulkanExampleBase *example, bool updateFrameGraph);

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();
	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);
};
