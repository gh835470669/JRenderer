#pragma once

#include <stb/stb_image.h>
#include <string>

namespace jre
{
    class STBImage
    {
    private:
        int m_width;
        int m_height;
        int m_channels;
        stbi_uc *m_data;

    public:
        STBImage(const std::string &file_name);
        STBImage(const STBImage &);
        STBImage &operator=(const STBImage &) noexcept;
        STBImage(STBImage &&other);
        STBImage &operator=(STBImage &&other) noexcept;
        ~STBImage();
        uint32_t width() const { return m_width; }
        uint32_t height() const { return m_height; }
        uint32_t channels() const { return m_channels; }
        const void *data() const { return m_data; }
        size_t data_size() const { return m_width * m_height * m_channels; }
    };
}