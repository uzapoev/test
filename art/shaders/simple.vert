#version 450
//precision lowp float;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    float _b[2];
    mat3 nmat;
    vec4 bones[128];
} ubo;

void main()
{
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);// + skin.positions[0];
    o_color = a_color;
#if VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}