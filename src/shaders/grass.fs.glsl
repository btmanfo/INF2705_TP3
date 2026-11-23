#version 400 core

in ATTRIBS_GS_OUT
{
    vec2 texCoord;
} attribsIn;

uniform sampler2D wheatTexture;
out vec4 FragColor;

void main()
{
    vec4 texel = texture(wheatTexture, attribsIn.texCoord);
    if (texel.a < 0.25)
        discard;

    vec3 tint = mix(vec3(0.45, 0.50, 0.22), vec3(1.0, 0.95, 0.70), attribsIn.texCoord.y);
    FragColor = vec4(texel.rgb * tint, 1.0);
}
