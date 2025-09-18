#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

// basic transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;    // world position
out vec3 fragNormal; // world normal

// Toggle to flip normals if your mesh has inward-facing normals
// Uncomment to enable the fix
// #define FLIP_NORMALS

void main() {
	vec4 worldPos = model * vec4(aPos, 1.0);
	fragPos = worldPos.xyz;

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	vec3 n = normalize(normalMatrix * aNormal);
#ifdef FLIP_NORMALS
	n = -n;
#endif
	fragNormal = n;

	gl_Position = projection * view * worldPos;
}
