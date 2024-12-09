#pragma once

#include <map>
#include <vector>
#include "jrenderer/asset/convert.hpp"
#include <vulkan/vulkan.hpp>

namespace jre
{
    class bytes : public std::vector<std::byte>
    {
    public:
        bytes() : std::vector<std::byte>() {}

        template <class T>
        bytes(const T &data) : std::vector<std::byte>(reinterpret_cast<const std::byte *>(&data), reinterpret_cast<const std::byte *>(&data) + sizeof(T)) {}

        operator std::vector<std::byte>() const { return *this; }
    };

    class SpecializationConstants
    {
    public:
        SpecializationConstants() = default;
        SpecializationConstants(const std::map<uint32_t, bytes> &constants) : m_constants(std::move(constants)) {};
        ~SpecializationConstants() = default;

        void reset() { m_constants.clear(); }

        template <class T>
        void set_constant(uint32_t constant_id, const T &data)
        {
            set_constant(constant_id, std::move(bytes(data)));
        }

        void set_constant(uint32_t constant_id, const bytes &data) { m_constants[constant_id] = data; }

        const std::map<uint32_t, bytes> &constants() const { return m_constants; }

    private:
        std::map<uint32_t, bytes> m_constants;
    };

    template <>
    inline std::pair<bytes, std::vector<vk::SpecializationMapEntry>> convert_to(const SpecializationConstants &specialization_constants)
    {
        bytes data;
        std::vector<vk::SpecializationMapEntry> entries;
        for (auto &[constant_id, constant] : specialization_constants.constants())
        {
            data.insert(data.end(), constant.begin(), constant.end());
            entries.emplace_back(constant_id, static_cast<uint32_t>(data.size() - constant.size()), constant.size());
        }
        return {std::move(data), std::move(entries)};
    }
}