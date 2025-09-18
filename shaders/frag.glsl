#version 460 core

in vec3 fragPos;    // world position
in vec3 fragNormal; // world normal

out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};
uniform Light light;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
};
uniform Material material;

uniform vec3 viewPos;

const float PI = 3.14159265359;

// Diagnostics/toggles:
// Uncomment to invert the light direction if your content uses the opposite convention
// #define INVERT_LIGHT_DIR
// Uncomment to visualize normals as colors (red=+X, green=+Y, blue=+Z)
// #define DEBUG_NORMALS

// Fresnel (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Normal Distribution Function: GGX / Trowbridge-Reitz
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Geometry function: Schlick-GGX for one direction
float geometrySchlickGGX(float NdotX, float roughness)
{
    float a = max(roughness * roughness, 1e-4);
    float k = (a + 1.0);
    k = (k * k) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// Smith's method for both view and light
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);

#ifdef DEBUG_NORMALS
    // Visualize normals as RGB colors
    vec3 debugColor = N * 0.5 + 0.5; // map [-1,1] to [0,1]
    FragColor = vec4(debugColor, 1.0);
    return;
#endif

    vec3 L = normalize(light.position - fragPos);
#ifdef INVERT_LIGHT_DIR
    L = -L;
#endif
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // distance attenuation
    float distance   = length(light.position - fragPos);
    float attenuation = 1.0 / max(distance * distance, 1e-4);
    vec3 radiance    = light.color * light.intensity * attenuation;

    // base reflectance
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);

    vec3 Lo = vec3(0.0);
    if (NdotL > 0.0)
    {
        vec3 H = normalize(V + L);
        float NDF = distributionGGX(N, H, material.roughness);
        float G   = geometrySmith(N, V, L, material.roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 1e-4);
        vec3 specular     = numerator / denominator;

        // energy-conserving diffuse
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - material.metallic);

        // Lambertian diffuse with PI normalization
        vec3 diffuse = kD * material.albedo / PI;

        Lo = (diffuse + specular) * radiance * NdotL;
    }

    // simple ambient term (no IBL)
    vec3 ambient = vec3(0.03) * material.albedo;

    vec3 color = ambient + Lo;
    // gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
