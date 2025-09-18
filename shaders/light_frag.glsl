#version 460 core

in vec3 fragPos;
out vec4 FragColor;

uniform vec3 color;

void main() {
    // Simple unlit/emissive color. No lighting calculations.
    FragColor = vec4(color, 1.0);
}
