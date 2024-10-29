#pragma once

#include "jrenderer/mesh.h"
#include "jrenderer/texture.h"
#include "jrenderer/render_set.h"
#include <glm/glm.hpp>

namespace jre
{
    class Model : public IRenderSetObject<Vertex, uint32_t>
    {
    public:
        glm::mat4 model_matrix = glm::identity<glm::mat4>();
        std::shared_ptr<const DefaultMesh> mesh;
        std::shared_ptr<const Texture2D> texture;

        Model(std::shared_ptr<const DefaultMesh> mesh, std::shared_ptr<const Texture2D> texture) : mesh(mesh), texture(texture) {}
        ~Model() = default;

        std::shared_ptr<const DefaultMesh> get_mesh() const override { return mesh; }
        glm::mat4 get_model_matrix() const override { return model_matrix; }
    };
}