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

uniform samplerCube environmentMap;
uniform float envMaxMips; // number of mip levels in the environment map
uniform float iblIntensity; // scales IBL contribution
uniform float horizonFadePower; // controls horizon fade exponent
uniform int debugMode;    // 0=off, 1=NdotL, 2=NdotV, 3=DirectOnly, 4=IBLOnly
uniform int enableIBL;    // 1=on, 0=off
uniform int enableDirect; // 1=on, 0=off
uniform float ao;         // ambient occlusion [0,1]

const float PI = 3.14159265359;

// #define DEBUG_NORMALS

// Fresnel (Schlick approximation)
// Approximates how much light is reflected vs refracted at a surface
// - cosTheta: cosine of the angle between view vector and half-vector (H·V)
// - F0: base reflectance at normal incidence (vec3 for RGB)
// Returns the Fresnel term F (vec3) which scales the specular color.
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    // Schlick's approximation: F = F0 + (1-F0)*(1-cosTheta)^5
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel variant for ambient/specular IBL that accounts for roughness
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// UE4/Epic approximation of the integrated BRDF (split-sum) for specular IBL
// Returns vec2(A, B) such that: spec = prefilteredEnv * (F0 * A + B)
vec2 envBRDFApprox(float NdotV, float roughness) {
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4( 1.0,  0.0425,  1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

// Specular occlusion approximation (Frostbite/EA)
float specularAO(float NdotV, float ao, float roughness) {
    float exponent = exp2(-16.0 * roughness - 1.0);
    return clamp(pow(NdotV + ao, exponent) - 1.0 + ao, 0.0, 1.0);
}

// ACES (approximate) tonemapping
vec3 tonemapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

// Normal Distribution Function (NDF): GGX / Trowbridge-Reitz
// The NDF describes the distribution of microfacet normals on the surface.
// - N: surface normal
// - H: half-vector between view and light (V+L)/|V+L|
// - roughness: material roughness in [0,1]
// Returns the scalar NDF value which controls the width/strength of the specular lobe.
float distributionGGX(vec3 N, vec3 H, float roughness) {
    // many implementations use alpha = roughness^2 to remap perceptual roughness
    float a  = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Geometry function (Schlick-GGX) for a single direction
// Approximates the masking-shadowing term for microfacets along one direction
// - NdotX: dot(N, X) where X is either V or L
// - roughness: material roughness
// Returns a scalar in [0,1] that attenuates the specular contribution.
float geometrySchlickGGX(float NdotX, float roughness) {
    // use roughness directly when computing k as in common references
    float a = max(roughness, 1e-4);
    float k = (a + 1.0);
    k = (k * k) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

// Smith's geometry function combines the per-direction Schlick-GGX terms
// to model both masking (visible microfacets from the view) and
// shadowing (microfacets occluded from light) simultaneously.
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

// No equirectangular sampling helpers needed when using a cubemap

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(viewPos - fragPos);

#ifdef DEBUG_NORMALS
    // Visualize normals as RGB colors
    vec3 debugColor = N * 0.5 + 0.5; // map [-1,1] to [0,1]
    FragColor = vec4(debugColor, 1.0);
    return;
#endif

    vec3 L = normalize(light.position - fragPos);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (debugMode == 1) { // visualize NdotL
        float v = NdotL;
        FragColor = vec4(vec3(v), 1.0);
        return;
    }
    if (debugMode == 2) { // visualize NdotV
        float v = NdotV;
        FragColor = vec4(vec3(v), 1.0);
        return;
    }

    // distance attenuation
    float distance   = length(light.position - fragPos);
    float attenuation = 1.0 / max(distance * distance, 1e-4);
    vec3 radiance    = light.color * light.intensity * attenuation;

    // base reflectance
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);

    vec3 Lo = vec3(0.0);
    if (NdotL > 0.0) {
        vec3 H = normalize(V + L);
        // --- Normal Distribution Function (NDF) ---
        // controls shape/width of the microfacet specular lobe
        float NDF = distributionGGX(N, H, material.roughness);

        // --- Geometry (masking-shadowing) ---
        // accounts for microfacet self-shadowing and masking
        float G   = geometrySmith(N, V, L, material.roughness);

        // --- Fresnel ---
        // how reflectance changes with angle (Schlick approximation)
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        // --- Specular (Cook-Torrance) ---
        // numerator: NDF * G * F
        // denominator: 4 * (N·V) * (N·L)
        vec3 numerator    = NDF * G * F;
        float denominator = max(4.0 * NdotV * NdotL, 1e-4);
        vec3 specular     = numerator / denominator;

        // --- Diffuse (Lambertian) ---
        // kS is the specular energy, kD is the diffuse portion (energy-conserving)
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - material.metallic);

        // Lambertian diffuse normalized by PI
        vec3 diffuse = kD * material.albedo / PI;

        // accumulate reflected radiance from this light (N·L for foreshortening)
        Lo = (diffuse + specular) * radiance * NdotL;
    }

    // Diffuse IBL (approximate irradiance): sample highest mip with normal direction
    float maxMips = (envMaxMips > 0.5) ? envMaxMips : 8.0;
    float maxMip = max(maxMips - 1.0, 0.0);
    vec3 irradiance = textureLod(environmentMap, N, maxMip).rgb;
    vec3 diffuseIBL = irradiance * material.albedo / PI;

    // --- Specular IBL (approximate) ---
    // Use reflection vector to sample the environment map for specular contribution.
    // A proper implementation would use a cubemap prefiltered mipmap chain and
    // a BRDF integration LUT. Here we approximate by sampling the same
    // equirectangular HDR with LOD driven by roughness.
    vec3 R = reflect(-V, N);
    // Convert roughness to a rough mip level (higher roughness -> blur -> higher mip)
    // We don't know the actual mipmap count for this sampler2D; assume 8 levels as a heuristic.
    float mip = material.roughness * maxMip;
    // sample with LOD (textureLod available in GLSL 4.60 core)
    vec3 prefiltered = textureLod(environmentMap, R, mip).rgb;

    // Specular IBL via split-sum approximation (Epic/UE4)
    float NoV = max(dot(N, V), 0.0);
    vec2 AB = envBRDFApprox(NoV, material.roughness);
    vec3 specularIBL = prefiltered * (F0 * AB.x + AB.y);
    // Horizon fade to prevent bright rims at grazing angles (occlude below-horizon reflection)
    float horizon = clamp(1.0 + dot(R, N), 0.0, 1.0);
    specularIBL *= pow(horizon, max(horizonFadePower, 0.0));
    // Additional visibility clamp based on view angle
    specularIBL *= geometrySchlickGGX(NoV, material.roughness);
    // Specular occlusion using AO and view angle
    float specAO = specularAO(NoV, ao, material.roughness);
    specularIBL *= specAO;

    // combine ambient diffuse and specular IBL
    // Energy-conserving split: kS + kD = 1, diffuse gets reduced by kS
    vec3 kS_ibl = fresnelSchlickRoughness(NoV, F0, material.roughness);
    vec3 kD_ibl = (vec3(1.0) - kS_ibl) * (1.0 - material.metallic);
    vec3 ambientCombined = ((diffuseIBL * ao) * kD_ibl + specularIBL) * iblIntensity;

    // final color before tone mapping/gamma, with debug overrides
    vec3 color = vec3(0.0);
    if (enableDirect != 0) color += Lo;
    if (enableIBL != 0)    color += ambientCombined;
    if (debugMode == 3) color = Lo;
    if (debugMode == 4) color = ambientCombined;

    // --- Tonemap and Gamma ---
    color = tonemapACES(color);
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
