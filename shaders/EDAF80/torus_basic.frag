#version 410

uniform sampler2D diffuse_texture;
uniform int has_diffuse_texture;

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
    
    // Specular
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    vec3 specular = spec * light_color * specular_color;
    
    // Ambient (simple)
    vec3 ambient = 0.1 * base_color;
    
    // Final color
    vec3 final_color = ambient + diffuse + specular + emissive_color;
    frag_color = vec4(final_color, 1.0);
}
