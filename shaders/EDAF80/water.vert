#version 410 core

layout (location = 0) in vec3 vertex;

layout (location = 2) in vec2 texCoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float t;

out VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec2 texCoord;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
    mat3 TBN;
} vs_out;

float wave(vec2 position, vec2 direction, float amplitude, float frequency,
           float phase, float sharpness, float time)
{
    return amplitude * pow(
        sin((position.x * direction.x + position.y * direction.y) * frequency + phase * time) * 0.5 + 0.5,
        sharpness
    );
}

vec2 waveDeriv(vec2 position, vec2 direction,
               float amplitude, float frequency,
               float phase, float sharpness, float time)
{
    float ang = (position.x * direction.x + position.y * direction.y) * frequency + phase * time;
    float s = sin(ang);
    float c = cos(ang);
    float inner = s * 0.5 + 0.5;
    float coeff = 0.5 * sharpness * frequency * amplitude * pow(inner, sharpness - 1.0) * c;
    return coeff * direction;
}

void main()
{
    vec3 displaced_vertex = vertex;
    displaced_vertex.y += wave(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
    displaced_vertex.y += wave(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, t);

    vs_out.vertex = (vertex_model_to_world * vec4(displaced_vertex, 1.0)).xyz;

    vec2 d_Hxz = vec2(0.0);
    d_Hxz += waveDeriv(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
    d_Hxz += waveDeriv(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, t);

    vec3 t_ms = normalize(vec3(1.0, d_Hxz.x, 0.0));
    vec3 b_ms = normalize(vec3(0.0, d_Hxz.y, 1.0));
    vec3 n_ms = normalize(cross(t_ms, b_ms));

    vec3 t_vec_ws = normalize((normal_model_to_world * vec4(t_ms, 0.0)).xyz);
    vec3 b_vec_ws = normalize((normal_model_to_world * vec4(b_ms, 0.0)).xyz);
    vec3 n_vec_ws = normalize((normal_model_to_world * vec4(n_ms, 0.0)).xyz);

    vs_out.TBN = mat3(t_vec_ws, b_vec_ws, n_vec_ws);
    vs_out.normal = n_vec_ws;
    vs_out.texCoord = texCoord;

    vec2 texScale = vec2(8.0, 4.0);
    float normalTime = mod(t, 100.0);
    vec2 normalSpeed = vec2(-0.05, 0.0);
    vs_out.normalCoord0 = texCoord * texScale + normalTime * normalSpeed;
    vs_out.normalCoord1 = texCoord * (texScale * 2.0) + normalTime * (normalSpeed * 4.0);
    vs_out.normalCoord2 = texCoord * (texScale * 4.0) + normalTime * (normalSpeed * 8.0);

    gl_Position = vertex_world_to_clip * vec4(vs_out.vertex, 1.0);
}
