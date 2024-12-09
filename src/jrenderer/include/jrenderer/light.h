#pragma once

#include <glm/glm.hpp>
#include "asset/convert.hpp"

namespace jre
{
    struct UniformLight
    {
        glm::vec4 position;  // position.w represents type of light  (0 = directional, 1 = point, 2 = spot)
        glm::vec4 color;     // color.w represents light intensity
        glm::vec4 direction; // direction.w represents range
        glm::vec2 info;      // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
    };

    struct UniformLightData
    {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    };

    struct DirectionalLight
    {

    public:
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;

        glm::vec3 direction() const { return m_direction; }
        void set_direction(const glm::vec3 &direction) { m_direction = glm::normalize(direction); }

    private:
        glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
    };

    template <>
    inline UniformLight convert_to(const DirectionalLight &light)
    {
        return {{}, {light.color, light.intensity}, {light.direction(), 0.0f}, {0, 0}};
    }
}
