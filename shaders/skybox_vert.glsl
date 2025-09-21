#version 460 core
layout(location = 0) in vec3 aPos;
out vec3 vDir;
uniform mat4 view;
uniform mat4 projection;
void main() {
	vDir = aPos;
	mat4 viewNoTrans = view;
	viewNoTrans[3] = vec4(0,0,0,viewNoTrans[3].w);
	vec4 clip = projection * viewNoTrans * vec4(aPos, 1.0);
	gl_Position = clip.xyww;
}
