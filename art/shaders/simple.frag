#version 450

layout(location = 0) in vec4 color;

layout(location = 0) out vec4 outColor;
//layout(binding = 0) uniform sampler2D texSampler;

void main()
{
//    vec4 c = texture(texSampler, gl_FragCoord.xy);
    outColor = vec4(color.r, color.g, 0.0, 1.0);
}