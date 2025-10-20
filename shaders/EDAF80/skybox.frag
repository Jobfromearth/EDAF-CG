#version 410 core
in VS_OUT { vec3 dir_ws; } fs_in;
out vec4 frag_color;

uniform samplerCube cubemap;

void main()
{
    frag_color = texture(cubemap, normalize(fs_in.dir_ws));
}
