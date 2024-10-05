
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Vectors
// Each of the scalar types, including booleans, have 2, 3, and 4-component vector equivalents. The n digit below can be 2, 3, or 4:
// bvecn: a vector of booleans
// ivecn: a vector of signed integers
// uvecn: a vector of unsigned integers
// vecn: a vector of single-precision floating-point numbers
// dvecn: a vector of double-precision floating-point numbers

// 如果出现dvec  location会占两个，是multiple slots。那如果是bvec 呢？可能不会少因为至少1
// layout(location = 0) in dvec3 inPosition;
// layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}