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

vec3 point_light_dir(vec3 pos, int i) {
    // Point light - direction vector
    // Position of the light in <gubo.lightPos[i]>
    return normalize(gubo.lightPos[i] - pos);
}

vec3 point_light_color(vec3 pos, int i) {
    // Point light - color
    // Color of the light in <gubo.lightColor[i].rgb>
    // Scaling factor g in <gubo.lightColor[i].a>
    // Decay power beta: constant and fixed to 2.0
    // Position of the light in <gubo.lightPos[i]>
    float distance = clamp(distance(gubo.lightPos[i],pos),0.0f,1.0f);
    float value = gubo.lightColor.a / distance;
    float intensity = pow(value, 2.0);
    vec3 result = gubo.lightColor.rgb * intensity;
    return result;
}

vec3 spot_light_dir(vec3 pos, int i) {
    // Spot light - direction vector
    // Direction of the light in <gubo.lightDir[i]>
    // Position of the light in <gubo.lightPos[i]>
    return point_light_dir(pos, i);
}

vec3 spot_light_color(vec3 pos, int i) {
    // Spot light - color
    // Color of the light in <gubo.lightColor[i].rgb>
    // Scaling factor g in <gubo.lightColor[i].a>
    // Decay power beta: constant and fixed to 2.0
    // Position of the light in <gubo.lightPos[i]>

    // Direction of the light in <gubo.lightDir[i]>
    // Cosine of half of the inner angle in <gubo.cosIn>
    // Cosine of half of the outer angle in <gubo.cosOut>
    vec3 L0 = point_light_color(pos, i);
    vec3 lx = point_light_dir(pos, i);
    return  L0 * clamp((dot(lx,gubo.lightDir[i]) - 0.00005f)/(0.5f - 0.05f), 0, 1);
}

vec3 BRDF(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {
	// Compute the BRDF, with a given color <Albedo>, in a given position characterized bu a given normal vector <Norm>,
	// for a light direct according to <LD>, and viewed from a direction <EyeDir>
	vec3 Diffuse;
	vec3 Specular;
	Diffuse = Albedo * max(dot(Norm, LD),0.0f);
	Specular = vec3(pow(max(dot(EyeDir, -reflect(LD, Norm)),0.0f), 160.0f));
	
	return Diffuse + Specular;
}

void main() {	
	if (gubo.currRoom != 1){
		discard;
	}
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);

	vec3 lightDir = spot_light_dir(fragPos,1);
	vec3 lightColor = spot_light_color(fragPos,1);

	vec3 computedColor = BRDF(vec3(1.0f,0.0f,0.0f),Norm,EyeDir,lightDir) * lightColor;

	outColor = vec4(computedColor,1.0f);
}



    