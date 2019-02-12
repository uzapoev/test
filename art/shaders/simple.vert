#version 450
layout(location = 0) in vec4 position;
//layout(location = 1) in vec4 color;
//layout(location = 0) out vec4 ocolor;

//layout(binding = 0)  uniform UniformBufferObject

void main()
{
    gl_Position = vec4(position.x, position.y, 0.0, 1.0);// + skin.positions[0];
}