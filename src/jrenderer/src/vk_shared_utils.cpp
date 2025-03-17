#include "jrenderer/utils/vk_shared_utils.h"

#include <vector>
#include <vulkan/vulkan_shared.hpp>
#include <ranges>
#include <fstream>
#include <iostream>

namespace vk
{
    namespace shared
    {
        std::vector<vk::SharedCommandBuffer> allocate_command_buffers(vk::SharedCommandPool command_pool,
                                                                      uint32_t count,
                                                                      vk::CommandBufferLevel level)
        {
            vk::SharedDevice device = command_pool.getDestructorType();
            return device->allocateCommandBuffers(
                       vk::CommandBufferAllocateInfo(
                           command_pool.get(),
                           level,
                           count)) |
                   std::views::transform([device, command_pool](vk::CommandBuffer buffer)
                                         { return vk::SharedCommandBuffer(buffer, device, command_pool); }) |
                   std::ranges::to<std::vector>();
        }

        static std::vector<vk::DescriptorPoolSize> count_pool_sizes(std::span<const vk::DescriptorSetLayoutBinding> bindings)
        {
            std::vector<vk::DescriptorPoolSize> pool_sizes;
            for (const auto &binding : bindings)
            {
                auto pool_size_iter = pool_sizes.begin();
                for (; pool_size_iter != pool_sizes.end(); ++pool_size_iter)
                {
                    if (pool_size_iter->type == binding.descriptorType)
                    {
                        pool_size_iter->descriptorCount++;
                        break;
                    }
                }
                if (pool_size_iter == pool_sizes.end())
                {
                    pool_sizes.push_back({binding.descriptorType, 1});
                }
            }
            return pool_sizes;
        }

        std::tuple<vk::SharedDescriptorPool, vk::SharedDescriptorSetLayout> make_descriptor_pool_with_layout(
            vk::SharedDevice device,
            uint32_t max_sets,
            vk::ArrayProxy<vk::DescriptorSetLayoutBinding> bindings)
        {
            std::vector<vk::DescriptorPoolSize> pool_sizes = count_pool_sizes(bindings);
            return {vk::SharedDescriptorPool(
                        device->createDescriptorPool(
                            vk::DescriptorPoolCreateInfo(
                                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                max_sets,
                                pool_sizes)),
                        device),
                    vk::SharedDescriptorSetLayout(
                        device->createDescriptorSetLayout(
                            vk::DescriptorSetLayoutCreateInfo(
                                {},
                                bindings)),
                        device)};
        }

        std::vector<vk::SharedDescriptorSet> allocate_descriptor_sets(vk::SharedDescriptorPool descriptor_pool,
                                                                      const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts)
        {
            return descriptor_pool.getDestructorType()->allocateDescriptorSets(
                       vk::DescriptorSetAllocateInfo(
                           descriptor_pool.get(),
                           descriptor_set_layouts)) |
                   std::views::transform([descriptor_pool](vk::DescriptorSet set)
                                         { return vk::SharedDescriptorSet(set, descriptor_pool.getDestructorType(), descriptor_pool); }) |
                   std::ranges::to<std::vector>();
        }
        std::vector<char> load_spv_shader_file(const std::string &file_path)
        {
            std::ifstream file(file_path, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                throw std::runtime_error("failed to open file!");
            }

            // std::ios::ate, Start reading at the end of the file
            // The advantage of starting to read at the end of the file is that we can use the read position to determine the size of the file and allocate a buffer:
            // https://cplusplus.com/reference/istream/istream/tellg/
            // Returns the position of the current character in the input stream.
            size_t file_size = (size_t)file.tellg();
            std::vector<char> buffer(file_size);

            file.seekg(0, file.beg);
            file.read(buffer.data(), file_size);
            file.close();

            return buffer;
        }

        vk::SharedShaderModule create_shader_from_spv(vk::SharedDevice device, const std::vector<char> &content)
        {
            vk::ShaderModuleCreateInfo create_info{};
            create_info.codeSize = content.size();
            create_info.pCode = reinterpret_cast<const uint32_t *>(content.data());
            return vk::SharedShaderModule{device->createShaderModule(create_info), device};
        }
        vk::SharedShaderModule create_shader_from_spv_file(vk::SharedDevice device, const std::string &file_path)
        {
            return create_shader_from_spv(device, load_spv_shader_file(file_path));
        }

        vk::SharedPipelineLayout create_pipeline_layout(vk::SharedDevice device,
                                                        vk::ArrayProxy<vk::DescriptorSetLayout> descriptor_set_layouts,
                                                        vk::ArrayProxy<vk::PushConstantRange> push_constant_ranges)
        {

            vk::PipelineLayoutCreateInfo pipeline_layout_info{};
            pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
            pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
            pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
            pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();
            return vk::SharedPipelineLayout(device->createPipelineLayout(pipeline_layout_info), device);
        }
    }
} // namespace vk