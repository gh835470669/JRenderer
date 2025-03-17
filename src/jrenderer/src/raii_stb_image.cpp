#include "jrenderer/asset/raii_stb_image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION
#include <filesystem>

namespace jre
{
    STBImage::STBImage(const std::string &file_name)
    {
        m_data = stbi_load(file_name.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
        m_channels = 4;
        if (!m_data)
        {
            throw std::runtime_error("failed to load texture image!");
        }
    }

    STBImage::STBImage(const STBImage &other)
        : m_width(other.m_width),
          m_height(other.m_height),
          m_channels(other.m_channels),
          m_data(nullptr)
    {
        m_data = new stbi_uc[other.m_width * other.m_height * other.m_channels];
        std::memcpy(m_data, other.m_data, other.m_width * other.m_height * other.m_channels);
    }

    STBImage &STBImage::operator=(const STBImage &other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        return *this = STBImage(other);
    }

    STBImage::STBImage(STBImage &&other)
        : m_width(other.m_width),
          m_height(other.m_height),
          m_channels(other.m_channels),
          m_data(std::exchange(other.m_data, nullptr))
    {
    }

    STBImage &STBImage::operator=(STBImage &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        std::swap(m_width, other.m_width);
        std::swap(m_height, other.m_height);
        std::swap(m_channels, other.m_channels);
        std::swap(m_data, other.m_data);
        return *this;
    }

    STBImage::~STBImage()
    {
        if (m_data)
        {
            stbi_image_free(m_data);
        }
    }
}