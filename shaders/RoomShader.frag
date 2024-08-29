#version 450
#extension GL_ARB_separate_shader_objects : enable

// Questo definisce la variabile ricevuta dal Vertex Shader
// le posizioni devono corrispondere a quelle delle sue variabili out
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 room;

// Questo definisce il colore calcolato da questo shader. Generalmente è sempre location 0.
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform GlobalUniformBufferObject {
	vec3 lightPos[3];
	vec3 lightDir[3];
	int currRoom;
	vec4 lightColor;
	vec3 eyePos;
} gubo;

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
	if (room == vec2(gubo.currRoom)){ 
		vec3 Norm = normalize(fragNorm);
		vec3 EyeDir = normalize(gubo.eyePos - fragPos);

        vec3 lightColor;
        vec3 lightDir;
    
        //Room 2 uses the point light from the sphere, the others use a spot light
        if (room != vec2(2)) {
            lightDir = spot_light_dir(fragPos,int(room.x));
            lightColor = spot_light_color(fragPos,int(room.x));
        } else {
            lightDir = point_light_dir(fragPos,int(room.x));
            lightColor = point_light_color(fragPos,int(room.x));
        }

		vec3 computedColor;
        vec3 lightEmitterColor = {2,2,2};
		if (fragColor == lightEmitterColor) {
            computedColor = vec3(1,1,1);
        } else {
			computedColor = BRDF(fragColor,Norm,EyeDir,lightDir) * lightColor;
		}
		outColor = vec4(computedColor, 1.0f);
	} else {
		discard;
	}
}



    