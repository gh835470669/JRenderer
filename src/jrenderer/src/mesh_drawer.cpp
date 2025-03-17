#include "jrenderer/mesh_drawer.h"

namespace jre
{
    void draw_mesh(const IMesh *mesh, vk::CommandBuffer command_buffer, vk::ArrayProxy<vk::Rect2D> sub_mesh_scissors)
    {
        const RenderMeshData mesh_data = std::move(mesh->get_render_data());
        // offset 和 first_binding 是给instancing 用的，先不管
        std::vector<vk::DeviceSize> offsets(mesh_data.vertexes.size(), 0);
        command_buffer.bindVertexBuffers(0, 1, mesh_data.vertexes.data(), offsets.data());
        command_buffer.bindIndexBuffer(mesh_data.index_buffer, 0, mesh_data.index_type);

        auto cur_scissor = sub_mesh_scissors.begin();
        for (const auto &sub_mesh : mesh_data.sub_meshes)
        {
            if (cur_scissor != sub_mesh_scissors.end())
            {
                command_buffer.setScissor(0, *cur_scissor);
                ++cur_scissor;
            }
            command_buffer.drawIndexed(sub_mesh.index_count, 1, sub_mesh.index_offset, sub_mesh.vertex_offset, 0);
        }
    }

    void DiffMeshDrawer::draw(const IMesh *mesh, vk::CommandBuffer command_buffer, vk::ArrayProxy<vk::Rect2D> sub_mesh_scissors)
    {
        const RenderMeshData mesh_data = std::move(mesh->get_render_data());
        // offset 和 first_binding 是给instancing 用的，先不管
        std::vector<vk::DeviceSize> offsets(mesh_data.vertexes.size(), 0);
        if (vertex_buffers_diff.update(mesh_data.vertexes))
        {
            command_buffer.bindVertexBuffers(0, 1, mesh_data.vertexes.data(), offsets.data());
        }
        if (index_buffer_diff.update(mesh_data.index_buffer))
        {
            command_buffer.bindIndexBuffer(mesh_data.index_buffer, 0, mesh_data.index_type);
        }

        auto cur_scissor = sub_mesh_scissors.begin();
        for (const auto &sub_mesh : mesh_data.sub_meshes)
        {
            if (cur_scissor != sub_mesh_scissors.end())
            {
                command_buffer.setScissor(0, *cur_scissor);
                ++cur_scissor;
            }
            command_buffer.drawIndexed(sub_mesh.index_count, 1, sub_mesh.index_offset, sub_mesh.vertex_offset, 0);
        }
    }

    void DiffMeshBinder::bind(const RenderMeshData &mesh_data, vk::CommandBuffer command_buffer)
    {
        // offset 和 first_binding 是给instancing 用的，先不管
        std::vector<vk::DeviceSize> offsets(mesh_data.vertexes.size(), 0);
        if (vertex_buffers_diff.update(mesh_data.vertexes))
        {
            command_buffer.bindVertexBuffers(0, 1, mesh_data.vertexes.data(), offsets.data());
        }
        if (index_buffer_diff.update(mesh_data.index_buffer))
        {
            command_buffer.bindIndexBuffer(mesh_data.index_buffer, 0, mesh_data.index_type);
        }
    }
}