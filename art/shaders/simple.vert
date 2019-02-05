#version 450
layout(location = 0) in vec4 position;
//layout(location = 1) in vec4 color;
//layout(location = 0) out vec4 ocolor;

uniform UniformBufferObject
{
    mat4 mvp;
    vec4 time;
}ubo;
/*
uniform SkinnedUniformObject
{
    vec4 positions[128];
	vec4 rotations[128];
}skin;*/

void main()
{
    gl_Position = ubo.mvp*position;// + skin.positions[0];
}