#version 410

uniform sampler2D diffuse_texture;
uniform int has_diffuse_texture;

// Environment map (skybox) for ambient lighting
uniform samplerCube environment_map;

uniform vec3 light_position;
uniform vec3 light_color;
uniform vec3 camera_position;

uniform vec3 diffuse_color;
uniform vec3 specular_color;
uniform float shininess;
uniform vec3 emissive_color;

in VS_OUT {
    vec2 texcoord;
    vec3 world_pos;
    vec3 world_normal;
} fs_in;

out vec4 frag_color;

void main()
{
    // Base color from texture or constant
    vec3 base_color = diffuse_color;
    if (has_diffuse_texture != 0) {
        base_color *= texture(diffuse_texture, fs_in.texcoord).rgb;
    }
    
    // Lighting calculation (Phong model)
    vec3 normal = normalize(fs_in.world_normal);
    vec3 light_dir = normalize(light_position - fs_in.world_pos);
    vec3 view_dir = normalize(camera_position - fs_in.world_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    
    // Diffuse
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * light_color * base_color;
    
    // Specular (direct light)
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    vec3 specular_direct = spec * light_color * specular_color;

    // Environment specular (image-based): sample cubemap along reflection of view
    // Reflection vector points to environment direction seen from the surface
    vec3 env_reflect = reflect(-view_dir, normal);
    vec3 env_spec_color = texture(environment_map, env_reflect).rgb;
    // Fresnel-like mix to scale environment spec by view angle; simple Schlick approx (fixed F0 from specular_color)
    float F0 = clamp(max(max(specular_color.r, specular_color.g), specular_color.b), 0.0, 1.0);
    float cosTheta = clamp(dot(normal, view_dir), 0.0, 1.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    const float env_spec_intensity = 0.5;
    vec3 specular_env = env_spec_intensity * fresnel * env_spec_color;
    
    // Ambient from environment map (sample along surface normal)
    // Scale factor controls how strong the environment ambient is
    const float ambient_intensity = 0.25;
    vec3 env_color = texture(environment_map, normal).rgb;
    vec3 ambient = ambient_intensity * env_color * base_color;
    
    // Final color
    vec3 final_color = ambient + diffuse + specular_direct + specular_env + emissive_color;
    frag_color = vec4(final_color, 1.0);
}

