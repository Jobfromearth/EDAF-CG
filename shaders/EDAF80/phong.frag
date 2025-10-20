#version 410 core
in VS_OUT {
    vec3 pos_ws;
    vec3 n_ws;
    vec2 uv;
    mat3 TBN;
} fs_in;

out vec4 frag_color;

uniform int use_normal_mapping;
uniform vec3 light_position;
uniform vec3 camera_position;

// 材质
uniform vec3  material_ambient  = vec3(0.1);
uniform vec3  material_specular = vec3(1.0);
uniform float material_shininess = 64.0;

// 贴图（与 C++ add_texture 的名字完全一致）
uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;
uniform sampler2D roughness_texture;

uniform int has_textures;
uniform int has_diffuse_texture;

void main()
{
    // 1) 基本向量（世界空间）
    vec3 N = normalize(fs_in.n_ws);
    vec3 L = normalize(light_position - fs_in.pos_ws);
    vec3 V = normalize(camera_position - fs_in.pos_ws);

    // 2) 可选：法线贴图
    if (use_normal_mapping == 1) {
        vec3 n_t = texture(normal_texture, fs_in.uv).xyz * 2.0 - 1.0; // [-1,1]
        N = normalize(fs_in.TBN * n_t);
    }

    // 3) 漫反射基色：用贴图（否则用常量）
    vec3 kd = (has_diffuse_texture == 1)
              ? texture(diffuse_texture, fs_in.uv).rgb
              : vec3(0.7, 0.2, 0.4);

    // 4) 用 roughness 控制高光强度（粗糙→弱）
    float rough = texture(roughness_texture, fs_in.uv).r; // 0=光滑 1=粗糙
    vec3  ks    = mix(vec3(1.0), vec3(0.0), rough);

    // 5) Phong
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = kd * NdotL;

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), material_shininess);
    vec3 specular = ks * spec;

    vec3 ambient = material_ambient;

    vec3 color = ambient + diffuse + specular;
    frag_color = vec4(color, 1.0);
}
