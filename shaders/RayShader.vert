#version 450
#extension GL_ARB_separate_shader_objects : enable

// The attributes associated with each vertex.
// Their type and location must match the definition given in the
// corresponding Vertex Descriptor, and in turn, with the CPP data structure
layout(location = 0) in vec3 inPosition;

// this defines the variable passed to the Fragment Shader
// the locations must match the one of its in variables
layout(location = 0) out vec2 fragUV;


layout(set = 1, binding = 0) uniform UniformBufferObject {
	vec3 cameraPos; // Posizione della camera
	mat4 inviewMatrix; // Matrice di vista inversa
	mat4 invProjectionMatrix; // Matrice di proiezione inversa
	int currBox;
} ubo;

// Here the shader simply computes clipping coordinates, and passes to the Fragment Shader
// the position of the point in World Space, the transformed direction of the normal vector,
// and the untouched (but interpolated) UV coordinates
void main() {
	fragUV = inPosition.xy * 0.5 + 0.5; // Converti da coordinate NDC a [0, 1]
    gl_Position = vec4(inPosition, 1.0); // Passa la posizione del vertice a clip space
}
