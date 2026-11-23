#version 400 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out ATTRIBS_GS_OUT
{
    vec2 texCoord;
} attribsOut;

uniform mat4 mvp;

float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void emitCorner(vec3 root, vec3 right, vec3 up, vec2 uv)
{
    attribsOut.texCoord = uv;
    gl_Position = mvp * vec4(root + (uv.x - 0.5) * right + uv.y * up, 1.0);
    EmitVertex();
}

void main()
{
    vec3 root = gl_in[0].gl_Position.xyz;
    float width = 0.5 + 0.1 * rand(root.xz + vec2(13.7, 5.2));
    float height = 2.4 + 0.5 * rand(root.xz + vec2(8.3, 41.1));
    float yaw = 6.28318530718 * rand(root.xz + vec2(32.1, 2.7));
    float tilt = 0.31415926536 * rand(root.xz + vec2(4.6, 19.3));

    vec3 right = width * vec3(cos(yaw), 0.0, -sin(yaw));
    vec3 up = height * vec3(sin(yaw) * sin(tilt), cos(tilt), cos(yaw) * sin(tilt));
    emitCorner(root, right, up, vec2(0.0, 0.0));
    emitCorner(root, right, up, vec2(1.0, 0.0));
    emitCorner(root, right, up, vec2(0.0, 1.0));
    emitCorner(root, right, up, vec2(1.0, 1.0));
    EndPrimitive();
}
