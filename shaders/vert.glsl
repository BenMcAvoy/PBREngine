#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aTangent; // xyz tangent, w = handedness
layout(location = 3) in vec2 aUV;

// basic transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vWorldPos;   // world position
out vec3 vWorldN;     // world normal
out vec3 vWorldT;     // world tangent
out vec3 vWorldB;     // world bitangent
out vec2 vUV;

void main() {
	vec4 worldPos = model * vec4(aPos, 1.0);
	vWorldPos = worldPos.xyz;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 N = normalize(normalMatrix * aNormal);
	// Build tangent basis, falling back if tangents are missing/zero
	vec3 T = normalMatrix * aTangent.xyz;
	float tLen = length(T);
	if (tLen < 1e-5) {
		// Construct arbitrary T orthogonal to N
		vec3 up = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
		T = normalize(cross(up, N));
	} else {
		T = normalize(T);
	}
	vec3 B = normalize(cross(N, T)) * (aTangent.w < 0.0 ? -1.0 : 1.0);

	vWorldN = N;
	vWorldT = T;
	vWorldB = B;
	vUV = aUV;

	gl_Position = projection * view * worldPos;
}
