#version 430 core

layout(local_size_x = 64) in;

struct Particle
{
    vec3 position;
    float zOrientation;
    vec4 velocity;
    vec4 color;
    vec2 size;
    float timeToLive;
    float maxTimeToLive;
};

layout(std140, binding = 0) readonly restrict buffer ParticlesInputBlock
{
    Particle particles[];
} dataIn;

layout(std140, binding = 1) writeonly restrict buffer ParticlesOutputBlock
{
    Particle particles[];
} dataOut;

uniform float time;
uniform float deltaTime;
uniform vec3 emitterPosition;
uniform vec3 emitterDirection;
uniform uint particleCount;

float rand01(float seed)
{
    return fract(sin(dot(vec2(time * 100.0 + seed, float(gl_GlobalInvocationID.x)),
        vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    uint index = gl_GlobalInvocationID.x;
    if (index >= uint(dataIn.particles.length()))
        return;

    // Écrire aussi les emplacements inactifs : après l'échange des buffers,
    // une nouvelle particule doit trouver un temps de vie nul en entrée.
    if (index >= particleCount)
    {
        dataOut.particles[index] = Particle(vec3(0.0), 0.0, vec4(0.0),
            vec4(0.0), vec2(0.0), 0.0, 0.0);
        return;
    }

    Particle particle = dataIn.particles[index];
    if (particle.timeToLive <= 0.0)
    {
        particle.position = emitterPosition;
        particle.zOrientation = 6.28318530718 * rand01(0.0);
        particle.velocity = vec4(0.3 * emitterDirection + vec3(0.0, 0.2, 0.0), 0.0);
        particle.color = vec4(0.61, 0.52, 0.38, 0.0);
        particle.size = vec2(1.2);
        particle.maxTimeToLive = mix(2.5, 4.0, rand01(17.3));
        particle.timeToLive = particle.maxTimeToLive;
    }
    else
    {
        particle.timeToLive = max(0.0, particle.timeToLive - deltaTime);
        particle.position += particle.velocity.xyz * deltaTime;
        particle.zOrientation += 0.5 * deltaTime;
        float age = clamp(1.0 - particle.timeToLive / particle.maxTimeToLive, 0.0, 1.0);
        particle.color.rgb = mix(vec3(0.61, 0.52, 0.38), vec3(1.0), age);
        particle.color.a = 0.2 * smoothstep(0.0, 0.2, age)
            * (1.0 - smoothstep(0.8, 1.0, age));
        particle.size = vec2(mix(1.2, 2.5, age));
    }
    dataOut.particles[index] = particle;
}
