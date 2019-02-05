#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    vec4 c = texture(texSampler, gl_FragCoord.xy);
    outColor = vec4(0.4, 0.6, 0.8, 1.0);
}