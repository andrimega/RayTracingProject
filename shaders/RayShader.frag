#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NULL -1
#define SPHERE 0
#define PLANE 1

#define MAX_DEPTH 5 
#define N_OBJECTS 9

const float NON_DIELECTRIC_REFRACTIVE_INDEX = 1e-6; //indice di rifrazione da associare a elementi non dielettrici, di modo tale che il metodo di isTotalRiflession() ? sia sempre True per loro

// Questo definisce la variabile ricevuta dal Vertex Shader
// le posizioni devono corrispondere a quelle delle sue variabili out
layout(location = 0) in vec2 fragUV;

// Questo definisce il colore calcolato da questo shader. Generalmente è sempre location 0.
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
	int numberOfSamples;
} gubo;
	
layout(set = 0, binding = 1) uniform sampler2D tex;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    vec3 cameraPos; // Posizione della camera
    mat4 invViewMatrix; // Matrice di vista inversa
    mat4 invProjectionMatrix; // Matrice di proiezione inversa
	int currBox;
} ubo;

struct Ray {
    vec3 origin;
    vec3 direction;
};

/* struct per i materiali
    - color: colore associato all'oggetto
	- emissionColor: se è una luce = bianco, altrimenti = nero (nel caso di luce metti color = nero)
	- emissionStrength: potenza associata alla luce [0,1]
	- smoothness: indice per gli specchi, più vicino a 0 più sarà un diffuse material, più vicino a 1 e più sarà uno specchio [0,1] 
	- dieletricConstant: indice per i materiali dieletrici, se = 0 allora il materiale non è dieletrico, se > 1 allora questo è il suo indice 
	
Esempi di materiali
	- SPECCHIO: colore = bianco, smoothness > 0.0 (= 1.0 specchio perfettamente riflettente)
	- NON SPECCHIO: smoothness = 0.0
	- DIELETTRICO: dieletricConstant > 1.0, smoothness > 0.0 (perchè questo valore serve per decidere se il raggio incidente viene riflesso o rifratto)
	- NON DIELETTRICO: dieletricConstant < 1.0
	- DIFFUSE: colore = qualsiasi, emissionStrength = 0.0, emissionColor = nero
	- LUCE: colore = qualisasi, emissionStrength > 0.0, emissionColor != nero
*/
struct RayTracingMaterial{
	vec4 color; 
	vec4 emissionColor; 
	float emissionStrength; 
	float smoothness; 
	float dieletricConstant;
};

struct GeometryHit{ // questa struttura dati contiene le informazioni dell'oggetto che abbiamo colpito col raggio
	bool isHit;
	vec3 position;
	vec3 normal;
	RayTracingMaterial material;
	int index; //questo serve per sapere che indice ha la geometry nell'array di geometry (serve ad esempio nella refraction)
	bool frontFace; //serve per la rifrazione se sto colpendo la faccia interna o esterna di un oggetto
};

/* struct per le geometrie
Non tutti i componenti sono necessari per ogni elemento. Se non sono necessari basta settarli a 0 o a un qualsiasi altro valore
	SFERA
		- type = SPHERE
		- settare center e radius
	PIANO
		- type = PLANE
		- settare point e normal 
	se il piano è infinito, allora tutti gli altri parametri possono essere settati come vec3(0.0) o 0.0
	se il piano è finito
		- normal: deve essere uno dei tre assi, quindi vec3(1.0, 0.0, 0.0) o vec3(0.0, 1.0, 0.0) o vec3(0.0, 0.0, 1.0)
		- vWidth e VHeight: direzioni dei lati del rettangolo (gli altri due assi non scelti dalla normale)
		- width e height: lunghezze dei lati del rettangolo
	(ovviamente puoi anche selezionare gli assi con valori negativi (-1.0,0.0,0.0) per invertire le direzioni)
*/
struct Geometry {
	//caratteristiche comuni a tutti
    int type; //SPHERE o PLANE
	RayTracingMaterial material; //struct per il material
	
	// Solo per sfere
    vec3 center;   
    float radius; 
	
	// Solo per piani
    vec3 point;   
    vec3 normal;
	vec3 vWidth;
	vec3 vHeight;
	float width;  //se il piano è infinito settare width & height = 0.0
	float height;
};

// -------------------------- PCG -----------------------------
uint randomState;

// PCG (permuted congruential generator) www.pcg-random.org 
uint NextRandom(inout uint state) {
	state = state * 747796405 + 2891336453;
	uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

float RandomValue(inout uint state) {
	return NextRandom(state) / 4294967295.0; // 2^32 - 1
}

// Random value in normal distribution (with mean=0 and sd=1) https://stackoverflow.com/a/6178290
float RandomValueNormalDistribution(inout uint state) {
	float theta = 2 * 3.1415926 * RandomValue(state);
	float rho = sqrt(-2 * log(RandomValue(state)));
	return rho * cos(theta);
}

vec3 RandomDirection(inout uint state) {
	float x = RandomValueNormalDistribution(state);
	float y = RandomValueNormalDistribution(state);
	float z = RandomValueNormalDistribution(state);
	return normalize(vec3(x, y, z));
}

// Genera un punto casuale sul cerchio di raggio 1, usato per il jittering
// Non facciamo partire il prossimo raggio dal punto di hit, ma da un punto nel suo intorno
vec2 RandomPointInCircle(inout uint rngState){
	float angle = RandomValue(rngState) * 2 * 3.1415926; //otteniamo un angolo randomico della sfera
	vec2 pointOnCircle = vec2(cos(angle), sin(angle)); //trasformiamo l'angolo in coordinate 2D
	return pointOnCircle * sqrt(RandomValue(rngState)); //otteniamo un punto qualsiasi lungo il raggio che collega il centro al punto 2D
}

// ------------------- RAY INTERSECTION METHODS --------------------- 


// Funzione per calcolare l'intersezione tra un raggio e una sfera
bool intersectSphere(Ray ray, Geometry geom, out GeometryHit hit) {
    vec3 oc = ray.origin - geom.center; // distanza tra i due centri
	
    // coefficienti della formula quadratica per l'intersezione di un raggio con una sfera
    float a = dot(ray.direction, ray.direction); // a = 1
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - geom.radius * geom.radius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0) {
		float t0 = (-b - sqrt(discriminant)) / (2.0 * a);
        float t1 = (-b + sqrt(discriminant)) / (2.0 * a);
		
		float dst = -1.0; // prendi sempre la più piccola positiva
        if (t0 > 0.0) {
            dst = t0;
        }
        if (t1 > 0.0 && (dst < 0.0 || t1 < dst)) {
            dst = t1;
        }
        

        if (dst > 0.0) {
            hit.position = ray.origin + dst * ray.direction;
			hit.normal = normalize(hit.position - geom.center);
			hit.material = geom.material;
			hit.isHit = true;
            return true;
        }
    }
    return false;
}

void calculateRectangleVertices(Geometry geom, out vec3 A, out vec3 B, out vec3 C) { 
    A = geom.point; // bottom-left
    B = A + geom.width * geom.vWidth; // bottom-right
	C = A + geom.height * geom.vHeight; // top-left
}

// Funzione per intersezione del raggio con un piano infinito o finito
bool intersectPlane(Ray ray, Geometry geom, out GeometryHit hit) {
    float denom = dot(geom.normal, ray.direction); 

    if (abs(denom) < 0.0001f) { //verifichiamo che il raggio non sia parallelo al piano
        return false; 
    }

    vec3 rayPlane = geom.point - ray.origin; // Vettore dal punto del raggio al punto del piano
    float t = dot(rayPlane, geom.normal) / denom; // distanza
  
    if (t < 0) { // Se t è negativo, l'intersezione è dietro il raggio
        return false; 
    }
	
	vec3 hitPosition = ray.origin + t * ray.direction; 
	
	if(geom.width > 0.1 || geom.height > 0.1){ //piano non infinito	
		vec3 A, B, C;
		calculateRectangleVertices(geom, A, B, C);
		
		vec3 AB = B - A;
		vec3 AC = C - A;
		vec3 AM = hitPosition - A; //M è l'hitpoint
		
		float AMx = dot(AM, normalize(AB));
        float AMy = dot(AM, normalize(AC));
        float lengthAB = length(AB);
        float lengthAC = length(AC);
		
		if(AMx >= 0.0 && AMy >= 0.0 && AMx <= lengthAB && AMy <= lengthAC){
			hit.position = hitPosition;
			hit.normal = geom.normal;
			hit.material = geom.material;
			hit.isHit = true;
			return true;
		}
		
		return false;
	}

	hit.position = hitPosition;
	hit.normal = geom.normal;
	hit.material = geom.material;
	hit.isHit = true;
	return true;
}

// Scegliamo il metodo di calcolo dell'intersezione sulla base della tipologia di geometria
bool intersectGeometry(Ray ray, Geometry geom, inout GeometryHit hit) {
    if (geom.type == SPHERE) {
        return intersectSphere(ray, geom, hit);
    } else if (geom.type == PLANE) {
        return intersectPlane(ray, geom, hit);
    }
    return false; 
}

//------------------ RIFLECTION & REFRACTION --------------------
vec3 Myrefract(vec3 v, vec3 n, float eta) { //implementazione della legge di Snell per il riflesso dentro i vetri
    float cosi = dot(v, n);
    float k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (k < 0.0) {
        return vec3(0.0); // Riflessione totale interna
    } else {
        return eta * v - (eta * cosi + sqrt(k)) * n;
    }
}

float schlickApproximation(float cosTheta, float refrIndex){ //fa l'effetto di sfocatura dei punti più lontani degli oggetti riflessi sul vetro, a seconda del punto di vista (Fresnel effect)
	// Approssimazione di Schlick per la riflettanza
	float r0 = (1.0 - refrIndex) / (1.0 + refrIndex);
	r0 = r0 * r0;
	return r0 + (1.0 - r0) * pow((1.0 - cosTheta), 5.0);
}


//------------------ COLOR CALCULATION ---------------------
vec4 rayCasting(Ray ray, uint seed, Geometry[9] geometries){
	vec4 rayColor = vec4(0.0);
	vec3 rayAttenuation = vec3(1.0);

	for(int bounce = 0; bounce < MAX_DEPTH; bounce++){ // gestione della ricorsione
		GeometryHit closestHit;
		closestHit.isHit = false;
		float closestDist = 1e20;

		// troviamo l'oggetto più vicino che viene intersecato
		for (int i = 0; i < N_OBJECTS; i++) {
			if(geometries[i].type != NULL){
				GeometryHit hit;
				if (intersectGeometry(ray, geometries[i], hit)) {
					float dist = length(hit.position - ray.origin);
					if (dist < closestDist) {
						closestDist = dist;
						closestHit = hit;
						closestHit.frontFace = dot(ray.direction, hit.normal) < 0; //capire se sto colpendo la faccia esterna o interna
						closestHit.normal = closestHit.frontFace ? closestHit.normal : - closestHit.normal; //eventualmente inverti la normale se sei all'interno del materiale
					}
				}
			}	
		}

		if (closestHit.isHit) {
			if(closestHit.material.emissionStrength > 0.0) { //se il raggio incontra un materiale che emette luce possiamo uscire dal ciclo
				rayColor += vec4(rayAttenuation * closestHit.material.emissionColor.rgb * closestHit.material.emissionStrength, 1.0);
				break;
			}

			float reflectionProb = closestHit.material.smoothness;
			float refrIndex = closestHit.material.dieletricConstant;

			if (refrIndex > 1.0) { //materiale dielettrico (vetro)				
				float ri = closestHit.frontFace ? (1.0 / refrIndex) : refrIndex; //se sto colpendo la faccia esterna allora sto passando da vuoto 1.0 a materiale refrIndex, altrimenti da materiale a vuoto
				
				vec3 normalRayDir = normalize(ray.direction);
				float cosTheta = min(dot(-normalRayDir, closestHit.normal), 1.0);
				float sinTheta = sqrt(1.0 - cosTheta * cosTheta); //sin^2 + cos^2 = 1
				
				if(ri * sinTheta > 1.0 || schlickApproximation(cosTheta, ri) > RandomValue(randomState)){ //riflessione totale, totalmente fuori dall'oggetto, dipende dall'angolo di incidenza
					ray.direction = reflect(normalRayDir, closestHit.normal);
				}
				else{ //possibile rifrazione all'interno dell'oggetto
					ray.direction = Myrefract(normalRayDir, closestHit.normal, ri);
				}
			}
			else{
				vec3 specularDir = reflect(ray.direction, closestHit.normal);
				vec3 diffuseDir = normalize(closestHit.normal + RandomDirection(randomState));
				ray.direction = mix(diffuseDir, specularDir, reflectionProb); //linear interpolation tra direzione diffuse e direzione specular
			}

			ray.origin = closestHit.position + ray.direction * 0.001; //self intersection problem	
			rayAttenuation *= closestHit.material.color.rgb;				
		} else {
			break;
		}
	}

	return rayColor;
}


vec3 UVtoRayDirection(vec2 uv){
	vec2 ndc = uv * 2.0 - 1.0; // Convertiamo le coordinate UV in coordinate NDC (Normalized Device Coordinates), passiamo da un sistema [0,1] a [-1,1]
    vec4 clipCoords = vec4(ndc, -1.0, 1.0); // Creiamo un punto nello spazio partendo dalle coordinate di prima, con z = -1
    vec4 viewCoords = ubo.invProjectionMatrix * clipCoords; // Convertiamo le coordinate della clip in coordinate dello spazio della vista
    vec3 rayDirection = normalize(viewCoords.xyz);  // Normalizziamo le coordinate
    return normalize(vec3(ubo.invViewMatrix * vec4(rayDirection, 0.0))); // Trasformiamo la direzione del raggio nello spazio del mondo
}

void main() {	
	if (ubo.currBox >=3) {
		discard;
	}
	// Inizializzo seed randomici 
    uvec2 pixelCoords = uvec2(gl_FragCoord.xy);
    uint pixelIndex = pixelCoords.y * 1000 + pixelCoords.x;
    randomState = pixelIndex + (uint(gl_FragCoord.w) + gubo.numberOfSamples) * 719393u;

    // Creiamo il raggio
    Ray ray;
    ray.origin = ubo.cameraPos;
    ray.direction = UVtoRayDirection(fragUV);
	
	//Definizione materiali
	RayTracingMaterial materialLight;
	materialLight.color = vec4(0.0, 0.0, 0.0, 0.0);
	materialLight.emissionColor = vec4(1.0, 1.0, 1.0, 1.0); 
	materialLight.emissionStrength = 1; 
	materialLight.smoothness = 0.0;
	materialLight.dieletricConstant = NON_DIELECTRIC_REFRACTIVE_INDEX;
	
	RayTracingMaterial material;
	material.color = vec4(1.0, 1.0, 1.0, 1.0);
	material.emissionColor = vec4(0.0, 0.0, 0.0, 0.0); 
	material.emissionStrength = 0.0; 
	material.smoothness = 0.0;
	material.dieletricConstant = NON_DIELECTRIC_REFRACTIVE_INDEX;
	
	
	// Definizione di oggetti    
	
	Geometry boxA[9];
	//BOX1
	boxA[0] = Geometry(PLANE,  material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 0.0), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //basso box1
	boxA[1] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 0.0), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato sinistro box1
	boxA[1].material.color = vec4(1.0, 0.0, 0.0, 1.0);
	boxA[2] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 10.0), vec3(0.0f, 0.0f, -1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato destro box1
	boxA[2].material.color = vec4(0.0, 1.0, 0.0, 1.0);
	boxA[3] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(10.0, 0.0, 0.0), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //fondo box1
	boxA[4] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 10.0, 0.0), vec3(0.0f, -1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //alto box1//OGGETTI
	//NEL BOX1
	boxA[5] = Geometry(PLANE, materialLight, vec3(0.0), 0.0f, vec3(3.0, 9.99, 3.0), vec3(0.0f, -1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 4.0, 4.0); //luce
	boxA[6] = Geometry(SPHERE,  material, vec3(7.0f, 1.5f, 2.5f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //green ball
	boxA[6].material.color = vec4(0.0, 1.0, 0.0, 1.0);
	boxA[6].material.smoothness = 0.2;
	boxA[7] = Geometry(SPHERE,  material, vec3(7.0f, 1.5f, 7.5f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //red ball
	boxA[7].material.color = vec4(1.0, 0.0, 0.0, 1.0);
	boxA[7].material.smoothness = 0.9;
	boxA[8] = Geometry(NULL, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 0.0), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0, 0.0); //null box
	
	Geometry boxB[9];
	//BOX2
	boxB[0] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 12.0), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //basso box2
	boxB[1] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 12.0), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato sinistro box2
	boxB[1].material.color = vec4(1.0, 0.0, 0.0, 1.0);
	boxB[2] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 22.0), vec3(0.0f, 0.0f, -1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato destro box2
	boxB[2].material.color = vec4(0.0, 1.0, 0.0, 1.0);
	boxB[3] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(10.0, 0.0, 12.0), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //fondo box2
	boxB[4] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 10.0, 12.0), vec3(0.0f, -1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //alto box2
	//NEL BOX2
	boxB[5] = Geometry(PLANE, materialLight, vec3(0.0), 0.0f, vec3(3.0, 9.99, 15.0), vec3(0.0f, -1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 4.0, 4.0); //luce
	boxB[6] = Geometry(SPHERE,  material, vec3(7.0f, 1.5f, 14.5f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //green ball
	boxB[6].material.color = vec4(0.0, 1.0, 0.0, 1.0);
	boxB[6].material.smoothness = 0.2;
	boxB[7] = Geometry(SPHERE,  material, vec3(7.0f, 1.5f, 19.5f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //red ball
	boxB[7].material.color = vec4(1.0, 0.0, 0.0, 1.0);
	boxB[8].material.smoothness = 0.9;
	boxB[8] = Geometry(SPHERE,  material, vec3(3.0f, 1.5f, 17.0f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //transparent ball
	boxB[8].material.color = vec4(1.0, 1.0, 1.0, 0.0);
	boxB[8].material.dieletricConstant = 1.5;
	
	Geometry boxC[9];
	//BOX3
	boxC[0] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 24.0), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //basso box3
	boxC[1] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 24.0), vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato sinistro box3
	boxC[1].material.color = vec4(1.0, 1.0, 1.0, 1.0);
	boxC[1].material.smoothness = 1;
	boxC[2] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 34.0), vec3(0.0f, 0.0f, -1.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //lato destro box3
	boxC[2].material.color = vec4(1.0, 1.0, 1.0, 1.0);
	boxC[2].material.smoothness = 1;
	boxC[3] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(10.0, 0.0, 24.0), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f), 10.0, 10.0); //fondo box3
	boxC[3].material.color = vec4(0.0, 0.0, 1.0, 1.0);
	boxC[4] = Geometry(PLANE, material, vec3(0.0), 0.0f, vec3(0.0, 10.0, 24.0), vec3(0.0f, -1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), 10.0, 10.0); //alto box3	
	//NEL BOX3
	boxC[5] = Geometry(SPHERE,  materialLight, vec3(5.0f, 7.0f, 29.0f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //luce
	boxC[6] = Geometry(SPHERE,  material, vec3(5.0f, 2.64f, 29.0f), 1.5f, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0); //red ball
	boxC[6].material.color = vec4(1.0, 1.0, 0.0, 1.0);
	boxC[6].material.smoothness = 0.7;
	boxC[7] = Geometry(NULL, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 0.0), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0, 0.0); //null box
	boxC[8] = Geometry(NULL, material, vec3(0.0), 0.0f, vec3(0.0, 0.0, 0.0), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0, 0.0); //null box
	
	
	//scegli il box da visualizzare
	Geometry[9] chosenBox;
	if(ubo.currBox == 0){
		chosenBox = boxA;
	}
	else if(ubo.currBox == 1){
		chosenBox = boxB;
	}
	else{
		chosenBox = boxC;
	}
	
	//Per ogni pixel castiamo un ray
	vec4 totalLight = vec4(0.0f);
	int rayPerPixel = 1;
	if(gubo.numberOfSamples == 0){
		rayPerPixel = 1; //se vogliamo migliorare la qualità dell'immagine quando ci stiamo muovendo
	}
	for(int i = 0; i < rayPerPixel; i++){
		vec2 jitter = RandomPointInCircle(randomState) * 0.001;
		vec2 jitteredUV = fragUV + jitter;
		
		ray.direction = UVtoRayDirection(jitteredUV);
		
		totalLight += rayCasting(ray, randomState, chosenBox);
	}
	totalLight /= rayPerPixel;
	
	vec4 oldColor = vec4(texture(tex, fragUV).rgb, 1.0f);
	int N = min(10000, gubo.numberOfSamples); 
	outColor = (oldColor * (N) + totalLight) / (N+1); 
}
