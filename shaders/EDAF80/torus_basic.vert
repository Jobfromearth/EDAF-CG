#version 410 core

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

out VS_OUT {
    vec2 texcoord;
    vec3 world_pos;
    vec3 world_normal;
} vs_out;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

void main()
{
    // Transform vertex to world space
    vec4 world_pos = vertex_model_to_world * vec4(vertex, 1.0);
    vs_out.world_pos = world_pos.xyz;
    
    // Transform normal to world space
    vs_out.world_normal = normalize((normal_model_to_world * vec4(normal, 0.0)).xyz);
    
    // Pass texture coordinates
    vs_out.texcoord = texcoord.xy;
    
    // Transform to clip space
    gl_Position = vertex_world_to_clip * world_pos;
}
