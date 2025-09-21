#version 460 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uColor;
uniform float uExposure;

vec3 tonemapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

void main(){
    vec3 hdr = texture(uColor, vUV).rgb;
    vec3 mapped = tonemapACES(hdr * max(uExposure, 0.0));
    vec3 ldr = pow(mapped, vec3(1.0/2.2));
    FragColor = vec4(ldr, 1.0);
}
