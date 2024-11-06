#include "jrenderer/asset/obj_file.h"

namespace jre
{
    ObjFile::ObjFile(const std::string &file_name)
    {
        std::string warn, err;
        if (!tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, file_name.c_str()))
        {
            throw std::runtime_error(warn + err);
        }
    }

    template <>
    MeshData<Vertex, uint32_t> convert_to(const ObjFile &obj_file)
    {
        MeshData<Vertex, uint32_t> mesh_data;
        auto &attrib = obj_file.attrib();
        auto &shapes = obj_file.shapes();
        auto &materials = obj_file.materials();

        // 创建顶点缓冲区
        mesh_data.vertices.reserve(attrib.vertices.size() / 3);
        mesh_data.indices.reserve(std::accumulate(shapes.begin(), shapes.end(), 0, [](uint32_t acc, const tinyobj::shape_t &shape)
                                                  { return acc + static_cast<uint32_t>(shape.mesh.indices.size()); }));

        std::unordered_map<Vertex, uint32_t> unique_vertices{};
        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
                // if (index.normal_index >= 0)
                // {
                //     vertices[index.vertex_index].normal = {
                //         attrib.normals[3 * index.normal_index + 0],
                //         attrib.normals[3 * index.normal_index + 1],
                //         attrib.normals[3 * index.normal_index + 2]};
                // }
                if (index.texcoord_index >= 0)
                {
                    vertex.tex_coord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                }
                vertex.color = {1.0f, 1.0f, 1.0f};
                if (unique_vertices.count(vertex) == 0)
                {
                    unique_vertices[vertex] = static_cast<uint32_t>(mesh_data.vertices.size());
                    mesh_data.vertices.push_back(vertex);
                }
                mesh_data.indices.push_back(unique_vertices[vertex]);
            }
        }
        return mesh_data;
    }
}
