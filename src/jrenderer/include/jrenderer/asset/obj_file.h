#pragma once

#include "jrenderer/mesh.h"
#include "jrenderer/asset/convert.hpp"
#include <numeric>

namespace jre
{
    class ObjFile
    {
    public:
        ObjFile(const std::string &file_name)
        {
            std::string warn, err;
            if (!tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, file_name.c_str()))
            {
                throw std::runtime_error(warn + err);
            }
        }

        const tinyobj::attrib_t &attrib() const { return m_attrib; }
        const std::vector<tinyobj::shape_t> &shapes() const { return m_shapes; }
        const std::vector<tinyobj::material_t> &materials() const { return m_materials; }

    private:
        tinyobj::attrib_t m_attrib;
        std::vector<tinyobj::shape_t> m_shapes;
        std::vector<tinyobj::material_t> m_materials;
    };
}