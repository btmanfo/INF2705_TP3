#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in ATTRIB_VS_OUT
{
    float zOrientation;
    vec4 color;
    vec2 size;
} attribIn[];

out ATTRIB_GS_OUT
{
    vec2 texCoord;
    vec4 color;
} attribOut;

uniform mat4 projection;

void emitCorner(vec2 uv, mat2 rotation)
{
    vec2 offset = rotation * ((uv - 0.5) * attribIn[0].size);
    vec4 position = gl_in[0].gl_Position + vec4(offset, 0.0, 0.0);
    gl_Position = projection * position;
    attribOut.texCoord = uv;
    attribOut.color = attribIn[0].color;
    EmitVertex();
}

void main()
{
    if (attribIn[0].color.a <= 0.0)
        return;

    float c = cos(attribIn[0].zOrientation);
    float s = sin(attribIn[0].zOrientation);
    mat2 rotation = mat2(c, s, -s, c);
    emitCorner(vec2(0.0, 0.0), rotation);
    emitCorner(vec2(1.0, 0.0), rotation);
    emitCorner(vec2(0.0, 1.0), rotation);
    emitCorner(vec2(1.0, 1.0), rotation);
    EndPrimitive();
}
