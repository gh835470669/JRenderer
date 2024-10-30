#pragma once

#include "jrenderer/mesh.h"
#include "jrenderer/texture.h"
#include "jrenderer/render_set.h"
#include "jrenderer/uniform_buffer.h"
#include <glm/glm.hpp>

namespace jre
{
    class DescriptorSet;
    class Model : public IRenderSetObject<Vertex, uint32_t>
    {
    public:
        glm::mat4 model_matrix = glm::identity<glm::mat4>();
        std::shared_ptr<const DefaultMesh> mesh;
        std::shared_ptr<const Texture2D> texture;
        std::unique_ptr<DescriptorSet> descriptor_set;
        std::unique_ptr<UniformBuffer<UniformBufferObject>> uniform_buffer;

        Model(std::shared_ptr<const DefaultMesh> mesh, std::shared_ptr<const Texture2D> texture, std::unique_ptr<DescriptorSet> descriptor_set, std::unique_ptr<UniformBuffer<UniformBufferObject>> uniform_buffer)
            : mesh(mesh), texture(texture), descriptor_set(std::move(descriptor_set)), uniform_buffer(std::move(uniform_buffer)) {}
        Model(const Model &) = default;
        Model &operator=(const Model &) = default;
        Model(Model &&) = default;
        Model &operator=(Model &&) = default;
        ~Model() = default;

        std::shared_ptr<const DefaultMesh> get_mesh() const override { return mesh; }
        glm::mat4 get_model_matrix() const override { return model_matrix; }
        const DescriptorSet &get_descriptor_set() const override { return *descriptor_set; }
        UniformBuffer<UniformBufferObject> &get_uniform_buffer() override { return *uniform_buffer; }
    };
}