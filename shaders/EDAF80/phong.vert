#version 410 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;   // x,y ��Ч
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

out VS_OUT {
    vec3 pos_ws;    // ����ռ�λ��
    vec3 n_ws;      // ����ռ䷨�ߣ���ֵǰ��
    vec2 uv;        // ��������
    mat3 TBN;       // ���߿ռ䵽ģ�Ϳռ�� TBN��������Ϊ t,b,n��
} vs_out;

uniform mat4 vertex_model_to_world;  // M
uniform mat4 normal_model_to_world;  // M^{-T}
uniform mat4 vertex_world_to_clip;   // VP

void main()
{
    vec4 pos_w4 = vertex_model_to_world * vec4(vertex, 1.0);
    vs_out.pos_ws = pos_w4.xyz;

    // �����߻��ͷ��߶��䵽ģ�͡�����һ�µĿռ䣨t��b��n ��������w=0��
    vec3 t_ws = normalize((normal_model_to_world * vec4(tangent,  0.0)).xyz);
    vec3 b_ws = normalize((normal_model_to_world * vec4(binormal, 0.0)).xyz);
    vec3 n_ws = normalize((normal_model_to_world * vec4(normal,   0.0)).xyz);



    vs_out.n_ws = n_ws;
    vs_out.TBN = mat3(t_ws, b_ws, n_ws);
    vs_out.uv = texcoord.xy;

    gl_Position = vertex_world_to_clip * pos_w4;
}
