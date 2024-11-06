#pragma once

#include "jrenderer/mesh.h"
#include "jrenderer/asset/convert.hpp"
#include <numeric>

namespace jre
{
    class ObjFile
    {
    public:
        ObjFile(const std::string &file_name);
        ~ObjFile() = default;

        const tinyobj::attrib_t &attrib() const { return m_attrib; }
        const std::vector<tinyobj::shape_t> &shapes() const { return m_shapes; }
        const std::vector<tinyobj::material_t> &materials() const { return m_materials; }

    private:
        tinyobj::attrib_t m_attrib;
        std::vector<tinyobj::shape_t> m_shapes;
        std::vector<tinyobj::material_t> m_materials;
    };

    template <>
    MeshData<Vertex, uint32_t> convert_to(const ObjFile &obj_file);

    template <typename VertexType = Vertex, typename IndexType = uint32_t, std::enable_if_t<std::is_base_of<IVertex, VertexType>::value, int> = 0>
    class ObjLoader : public IMeshLoader<VertexType, IndexType>
    {
    public:
        MeshData<VertexType, IndexType> load(const std::string &name) override
        {
            const ObjFile obj_file(name);
            return convert_to<MeshData<Vertex, IndexType>, ObjFile>(obj_file);
        }
    };
}