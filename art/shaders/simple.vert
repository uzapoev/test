#version 450
//precision lowp float;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;

layout(location = 0) out vec4 o_color;

uniform UniformBufferObject 
{
    mat4 mvp;
    vec4 color;
} ubo;

uniform SkinInfo
{
    vec4 positions[128];
    vec4 rotations[128];
} skin_bo;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);// + skin.positions[0];
    o_color = a_color * ubo.color;
#if VULKAN
    gl_Position.y = -gl_Position.y;
#endif
}