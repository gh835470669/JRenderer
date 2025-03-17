#pragma once

#include "jrenderer/mesh.h"
#include "jrenderer/utils/diff_trigger.hpp"

namespace jre
{
    void draw_mesh(const IMesh *mesh, vk::CommandBuffer command_buffer, vk::ArrayProxy<vk::Rect2D> sub_mesh_scissors = {});

    class DiffMeshDrawer
    {
    public:
        DiffTrigger<std::vector<vk::Buffer>> vertex_buffers_diff;
        DiffTrigger<vk::Buffer> index_buffer_diff;

        void draw(const IMesh *mesh, vk::CommandBuffer command_buffer, vk::ArrayProxy<vk::Rect2D> sub_mesh_scissors = {});
    };

    class DiffMeshBinder
    {
    public:
        DiffTrigger<std::vector<vk::Buffer>> vertex_buffers_diff;
        DiffTrigger<vk::Buffer> index_buffer_diff;

        void bind(const RenderMeshData &mesh_data, vk::CommandBuffer command_buffer);
    };
}