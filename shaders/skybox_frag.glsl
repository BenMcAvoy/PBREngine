#version 460 core
in vec3 vDir;
out vec4 FragColor;
uniform samplerCube environmentMap;
void main() {
	vec3 col = texture(environmentMap, normalize(vDir)).rgb;
	// Assume env is already linear HDR; just tone map slightly for display if used directly
	const float a=2.51,b=0.03,c=2.43,d=0.59,e=0.14;
	vec3 t = clamp((col*(a*col+b))/(col*(c*col+d)+e),0.0,1.0);
	FragColor = vec4(pow(t, vec3(1.0/2.2)), 1.0);
}
