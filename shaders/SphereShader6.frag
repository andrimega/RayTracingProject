#version 450
#extension GL_ARB_separate_shader_objects : enable

// Questo definisce la variabile ricevuta dal Vertex Shader
// le posizioni devono corrispondere a quelle delle sue variabili out
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

// Questo definisce il colore calcolato da questo shader. Generalmente è sempre location 0.
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform GlobalUniformBufferObject {
	vec3 lightPos[3];
	vec3 lightDir[3];
	int currRoom;
	vec4 lightColor;
	vec3 eyePos;
} gubo;
layout(set = 0, binding = 1) uniform sampler2D tex;

void main() {	
	if (gubo.currRoom != 2){
		discard;
	}
	outColor = vec4(1.0f);
}



    