#version 460 core

in vec3 vWorldPos;
in vec3 vWorldN;
in vec3 vWorldT;
in vec3 vWorldB;
in vec2 vUV;

out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};
uniform Light light;

struct Material {
    // Constant fallbacks (set by C++ when no texture provided)
    vec3 Albedo;
    float Metallic;
    float Roughness;
    float AO;
    vec3 Emissive;
    // Texture toggles and samplers bound via macro in C++
    int hasAlbedoMap;    sampler2D AlbedoMap;
    int hasMetallicMap;  sampler2D MetallicMap;
    int hasRoughnessMap; sampler2D RoughnessMap;
    int hasAOMap;        sampler2D AOMap;
    int hasEmissiveMap;  sampler2D EmissiveMap;
};
uniform Material material;

uniform sampler2D u_NormalMap; // optional normal map
uniform int u_HasNormalMap;
uniform float u_AlphaCutoff;
uniform int u_DoubleSided;

uniform vec3 viewPos;

uniform samplerCube environmentMap;
uniform float envMaxMips; // number of mip levels in the environment map
uniform float iblIntensity; // scales IBL contribution
uniform float horizonFadePower; // controls horizon fade exponent
uniform int debugMode;    // 0=off, 1=NdotL, 2=NdotV, 3=DirectOnly, 4=IBLOnly
uniform int enableIBL;    // 1=on, 0=off
uniform int enableDirect; // 1=on, 0=off

const float PI = 3.14159265359;

// #define DEBUG_NORMALS

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec2 envBRDFApprox(float NdotV, float roughness) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4( 1.0,  0.0425,  1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

float specularAO(float NdotV, float ao, float roughness) {
    float exponent = exp2(-16.0 * roughness - 1.0);
    return clamp(pow(NdotV + ao, exponent) - 1.0 + ao, 0.0, 1.0);
}

vec3 tonemapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX(float NdotX, float roughness) {
    float a = max(roughness, 1e-4);
    float k = (a + 1.0);
    k = (k * k) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

vec3 sampleNormal(vec2 uv, vec3 N, vec3 T, vec3 B) {
    if (u_HasNormalMap == 0) {
        return normalize(N);
    }
    vec3 nTex = texture(u_NormalMap, uv).xyz * 2.0 - 1.0; // tangent-space normal
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
    vec3 nWorld = normalize(TBN * nTex);
    return nWorld;
}

void main() {
    vec3 Ngeom = normalize(vWorldN);
    vec3 N = sampleNormal(vUV, Ngeom, vWorldT, vWorldB);
    // Double-sided: flip normals if backfacing
    if (u_DoubleSided != 0) {
        vec3 Vdir = normalize(viewPos - vWorldPos);
        if (dot(N, Vdir) < 0.0) N = -N;
    }
    vec3 V = normalize(viewPos - vWorldPos);

#ifdef DEBUG_NORMALS
    vec3 debugColor = N * 0.5 + 0.5;
    FragColor = vec4(debugColor, 1.0);
    return;
#endif

    // Material parameter resolution (texture overrides constants)
    vec3 baseColor = material.Albedo;
    if (material.hasAlbedoMap != 0) {
        vec3 srgb = texture(material.AlbedoMap, vUV).rgb;
        baseColor = pow(max(srgb, vec3(0.0)), vec3(2.2)); // sRGB -> linear
    }
    float metallic = material.Metallic;
    if (material.hasMetallicMap != 0) metallic = texture(material.MetallicMap, vUV).b;
    float roughness = material.Roughness;
    if (material.hasRoughnessMap != 0) roughness = texture(material.RoughnessMap, vUV).g;
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0); // avoid 0 which causes fireflies
    float aoVal = material.AO;
    if (material.hasAOMap != 0) aoVal = texture(material.AOMap, vUV).r;
    vec3 emissive = material.Emissive;
    if (material.hasEmissiveMap != 0) {
        vec3 srgbE = texture(material.EmissiveMap, vUV).rgb;
        emissive = pow(max(srgbE, vec3(0.0)), vec3(2.2));
    }

    // Optional alpha cutoff using baseColor alpha if available (assume 1 if no alpha)
    float alpha = 1.0;
    if (material.hasAlbedoMap != 0) alpha = texture(material.AlbedoMap, vUV).a;
    if (alpha < u_AlphaCutoff) discard;

    vec3 L = normalize(light.position - vWorldPos);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (debugMode == 1) { FragColor = vec4(vec3(NdotL), 1.0); return; }
    if (debugMode == 2) { FragColor = vec4(vec3(NdotV), 1.0); return; }

    float distance   = length(light.position - vWorldPos);
    float attenuation = 1.0 / max(distance * distance, 1e-4);
    vec3 radiance    = light.color * light.intensity * attenuation;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);

    vec3 Lo = vec3(0.0);
    if (NdotL > 0.0) {
        vec3 H = normalize(V + L);
        float NDF = distributionGGX(N, H, roughness);
        float G   = geometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = max(4.0 * NdotV * NdotL, 1e-4);
        vec3 specular     = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = kD * baseColor / PI;
        Lo = (diffuse + specular) * radiance * NdotL;
    }

    float maxMips = (envMaxMips > 0.5) ? envMaxMips : 8.0;
    float maxMip = max(maxMips - 1.0, 0.0);
    vec3 irradiance = textureLod(environmentMap, N, maxMip).rgb;
    vec3 diffuseIBL = irradiance * baseColor / PI;

    vec3 R = reflect(-V, N);
    float mip = roughness * maxMip;
    vec3 prefiltered = textureLod(environmentMap, R, mip).rgb;

    float NoV = max(dot(N, V), 0.0);
    vec2 AB = envBRDFApprox(NoV, roughness);
    vec3 specularIBL = prefiltered * (F0 * AB.x + AB.y);
    float horizon = clamp(1.0 + dot(R, N), 0.0, 1.0);
    specularIBL *= pow(horizon, max(horizonFadePower, 0.0));
    specularIBL *= geometrySchlickGGX(NoV, roughness);
    float specAOv = specularAO(NoV, aoVal, roughness);
    specularIBL *= specAOv;

    vec3 kS_ibl = fresnelSchlickRoughness(NoV, F0, roughness);
    vec3 kD_ibl = (vec3(1.0) - kS_ibl) * (1.0 - metallic);
    vec3 ambientCombined = ((diffuseIBL * aoVal) * kD_ibl + specularIBL) * iblIntensity;

    vec3 color = emissive; // emissive adds directly
    if (enableDirect != 0) color += Lo;
    if (enableIBL != 0)    color += ambientCombined;
    if (debugMode == 3) color = Lo;
    if (debugMode == 4) color = ambientCombined;

    // Output linear HDR color. Tonemapping and gamma are applied later in the post-process pass.
    FragColor = vec4(color, 1.0);
}
