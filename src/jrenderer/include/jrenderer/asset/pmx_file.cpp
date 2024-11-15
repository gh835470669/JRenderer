#include "pmx_file.h"
#include "jrenderer/star_rail_char_render_set.h"

namespace jre
{
    PmxFile::PmxFile(const std::string &file_name)
    {
        std::ifstream stream = std::ifstream(file_name, std::ios_base::binary);
        m_model.Read(&stream);
    }

    template <>
    PmxMeshData convert_to(const PmxFile &pmx_file)
    {
        PmxMeshData mesh_data;
        const pmx::PmxModel &model = pmx_file.model();

        // vertex
        mesh_data.vertices.reserve(model.vertex_count);
        for (size_t i = 0; i < model.vertex_count; ++i)
        {
            const pmx::PmxVertex &vertex = model.vertices[i];
            mesh_data.vertices.emplace_back(jmath::vec3(vertex.positon[0], vertex.positon[1], vertex.positon[2]),
                                            jmath::vec3(vertex.normal[0], vertex.normal[1], vertex.normal[2]),
                                            jmath::vec2(vertex.uv[0], vertex.uv[1]));
        }

        // index
        mesh_data.indices.reserve(model.index_count);
        mesh_data.indices.assign(model.indices.get(), model.indices.get() + model.index_count);

        // sub mesh
        mesh_data.sub_mesh_indices.reserve(model.material_count);
        uint32_t index_offset = 0;
        for (size_t i = 0; i < model.material_count; ++i)
        {
            const pmx::PmxMaterial &material = model.materials[i];
            mesh_data.sub_mesh_indices.emplace_back(index_offset, material.index_count);
            index_offset += material.index_count;
        }
        return mesh_data;
    }

    template <>
    std::vector<PmxTextureName> convert_to(const PmxFile &pmx_file)
    {
        const pmx::PmxModel &model = pmx_file.model();
        std::vector<PmxTextureName> texture_names;
        texture_names.reserve(model.texture_count);
        for (size_t i = 0; i < model.texture_count; ++i)
        {
            texture_names.emplace_back(model.textures[i]);
        }
        return texture_names;
    }

}