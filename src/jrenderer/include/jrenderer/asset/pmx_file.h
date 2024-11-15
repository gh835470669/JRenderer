#pragma once

#include <string>
#include "Pmx.h"
#include "jrenderer/asset/convert.hpp"

namespace jre
{
    class PmxFile
    {
    public:
        PmxFile(const std::string &file_name);
        ~PmxFile() = default;

        const pmx::PmxModel &model() const { return m_model; }

    private:
        pmx::PmxModel m_model;
    };

    class PmxMeshData;
    template <>
    PmxMeshData convert_to(const PmxFile &pmx_file);

    using PmxTextureName = std::wstring;
    template <>
    std::vector<PmxTextureName> convert_to(const PmxFile &pmx_file);
}