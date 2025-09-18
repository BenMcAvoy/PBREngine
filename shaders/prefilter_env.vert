#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 localPos;

void main() {
    localPos = aPos;
    mat3 rotView = mat3(view);
    vec3 pos = rotView * aPos;
    gl_Position = projection * vec4(pos, 1.0);
}
