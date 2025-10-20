#version 410 core
layout (location = 0) in vec3 vertex;

out VS_OUT {
    vec3 dir_ws;
} vs_out;

uniform mat4 vertex_model_to_world; // M
uniform mat4 vertex_world_to_clip;  // VP

void main()
{
    // 大球顶点变到世界空间
    vec3 pos_ws = (vertex_model_to_world * vec4(vertex, 1.0)).xyz;

    // 用世界空间方向作为 cubemap 采样方向
    vs_out.dir_ws = pos_ws;

    // 常规投影
    gl_Position = vertex_world_to_clip * vec4(pos_ws, 1.0);
}
