#version 330 core

in ATTRIB_GS_OUT
{
    vec2 texCoord;
    vec4 color;
} attribIn;

out vec4 FragColor;
uniform sampler2D textureSampler;

void main()
{
    vec4 texel = texture(textureSampler, attribIn.texCoord);
    if (texel.a < 0.02)
        discard;
    FragColor = texel * attribIn.color;
}
