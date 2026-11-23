#version 400 core

layout(vertices = 3) out;
uniform mat4 modelView;

float edgeLevel(vec3 a, vec3 b)
{
    const float MIN_TESS = 2.0;
    const float MAX_TESS = 32.0;
    const float MIN_DIST = 10.0;
    const float MAX_DIST = 40.0;
    float distanceToCamera = length((modelView * vec4((a + b) * 0.5, 1.0)).xyz);
    float factor = smoothstep(MIN_DIST, MAX_DIST, distanceToCamera);
    float level = mix(MAX_TESS, MIN_TESS, factor);
    // Des paliers réduisent la fréquence des changements de densité.
    return exp2(ceil(log2(level)));
}

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] = edgeLevel(gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
        gl_TessLevelOuter[1] = edgeLevel(gl_in[2].gl_Position.xyz, gl_in[0].gl_Position.xyz);
        gl_TessLevelOuter[2] = edgeLevel(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz);
        gl_TessLevelInner[0] = max(gl_TessLevelOuter[0],
            max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));
    }
}
