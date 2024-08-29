// This has been adapted from the Vulkan tutorial

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"

#define n_objects 7

/////////////////////////////////////////////////////////////
///						GP Structs						  ///
/////////////////////////////////////////////////////////////
//UBO for the GP world matrix
struct TransformUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

//Lights, one for each box
struct LightUniformBufferObject {
	struct {
	alignas(16) glm::vec3 v;
	} lightPos[3];
	struct {
	alignas(16) glm::vec3 v;
	} lightDir[3];
	alignas(16) int currRoom;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

//Vertex for the room's walls
struct VertexRooms {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec3 color;
	glm::vec2 room;
};

//Vertex for the spheres
struct VertexSpheres {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

//Vertex for the mirrors in the third room
struct VertexMirrors {
	glm::vec3 pos;
	glm::vec2 UV;
};

/////////////////////////////////////////////////////////////
///						Ray Structs						  ///
/////////////////////////////////////////////////////////////
//UBO for Ray Tracing informations
struct UniformBufferObject {
	alignas(16) glm::vec3 cameraPos;
	alignas(16) glm::mat4 invViewMatrix;
	alignas(16) glm::mat4 invProjectionMatrix;
	alignas(16) int currBox;
};

//Used for progressive rendering
struct GlobalUniformBufferObject {
	alignas(16) int numberOfSamples;
};

//Vertex struct to pass to the vertex shader
struct Vertex {
	glm::vec3 pos;
};


/////////////////////////////////////////////////////////////
///						MAIN							  ///
/////////////////////////////////////////////////////////////
class App : public BaseProject {
protected:
	
	///////////		GP DLS,DS,Pipeline,Models,Textures	///////////
	// Descriptor Layouts
	DescriptorSetLayout DSLGlobalTransform;	
	DescriptorSetLayout DSLSphereTransform;	
	DescriptorSetLayout DSLlight;

	// Vertex formats
	VertexDescriptor VDRooms;
	VertexDescriptor VDSpheres;
	VertexDescriptor VDMirrors;

	// Pipelines [Shader couples]
	Pipeline Prooms, Pmirrors;
	Pipeline Psphere1,Psphere2,Psphere3,Psphere4,Psphere5,Psphere6,Psphere7;

	// Models, textures and Descriptor Sets (values assigned to the uniforms)
	Model Room1, Room2, Room3, Light1, Light2, MirrorL, MirrorR;
	Model S[n_objects];
	Texture T[n_objects];
	Texture TM;

	DescriptorSet DSLight,DSGlobalGP;
	DescriptorSet DSSphere[n_objects]; 

	///////////	  RAY DLS,DS,Pipeline,Models,Textures	///////////
	// Descriptor Layouts
	DescriptorSetLayout DSLglobal;	
	DescriptorSetLayout DSLray;	

	// Vertex formats
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline Pray;

	// Models, textures and Descriptor Sets (values assigned to the uniforms)
	Model Mtri;
	Texture Timage; //Previous frame texture

	DescriptorSet DSray, DSGlobal;

	///////////	  Other parameters	///////////
	int numberOfSamples = -1; //for progressive rendering
	int counter = 0; //used to alternate between rooms
	int currentBox = 0; //box currently being shown
	bool pressed = false; //used to read keyboard inputs

	//camera position and angles
	glm::vec3 CamPos = glm::vec3(-21.0, 5, 16.0);
	float CamAlpha = glm::radians(-75.0f);
	float CamBeta = glm::radians(-10.0f);
	float Ar;

	//Translation matrices for the spheres
	glm::mat4 Tpre[n_objects];

	//-----------------------------------------------------------
	//------------------------- METHODS -------------------------
	//-----------------------------------------------------------

	int nextBox(int i) {
		int sequence[] = {0,3,1,4,2,5};
		return sequence[i];
	}

	//Populate the GP models
	void initModels(){
		//Stanza 1
		std::vector<glm::vec3> vertices1 = {
			//Floor
			{0.0f,  0.0f, 0.0f},  
			{10.0f, 0.0f, 0.0f},  
			{0.0f, 0.0f, 10.0f},  
			{10.0f, 0.0f, 10.0f},
			//Left
			{0.0f,  0.0f, 0.0f},  
			{0.0f, 10.0f, 0.0f},  
			{10.0f, 0.0f, 0.0f},  
			{10.0f, 10.0f, 0.0f},
			//Right
			{0.0f,  0.0f, 10.0f},  
			{0.0f, 10.0f, 10.0f},  
			{10.0f, 0.0f, 10.0f},  
			{10.0f, 10.0f, 10.0f},
			//Back
			{10.0f,  0.0f, 0.0f},  
			{10.0f, 10.0f, 0.0f},  
			{10.0f, 0.0f, 10.0f},  
			{10.0f, 10.0f, 10.0f},
			//Top
			{0.0f, 10.0f, 0.0f}, //16 
			{10.0f, 10.0f, 0.0f},  
			{0.0f, 10.0f, 10.0f},  
			{10.0f, 10.0f, 10.0f}, //19
			{3.0, 10, 3.0}, //20
			{7.0, 10, 3.0},  
			{3.0, 10, 7.0},  
			{7.0, 10, 7.0}, //23
			{0.0f,10.0f,3.0f}, //24
			{0.0f,10.0f,7.0f},
			{10.0f,10.0f,3.0f},
			{10.0f,10.0f,7.0f} //27
		};
		std::vector<uint32_t> indices1 = {
			//Floor
			0, 1, 2,
			1, 2, 3,
			//Left
			4, 5, 6,
			5, 6, 7,
			//Right
			8, 9, 10,
			9, 10, 11,
			//Back
			12, 13, 14,
			13, 14, 15,
			//Top
			16, 24, 26,
			16, 26, 17,
			25, 18, 19,
			25, 19, 27,
			24, 25, 22,
			24, 22, 20,
			21, 23, 27,
			21, 26, 27
		};
		//Stanza 2
		std::vector<glm::vec3> vertices2 = {
			//Floor
			{0.0f, 0.0f, 12.0f},  
			{10.0f, 0.0f, 12.0f},  
			{0.0f, 0.0f, 22.0f},  
			{10.0f, 0.0f, 22.0f},
			//Left
			{0.0f, 0.0f, 12.0f},  
			{10.0f, 0.0f, 12.0f},  
			{0.0f, 10.0f, 12.0f},  
			{10.0f, 10.0f, 12.0f},
			//Right
			{0.0f, 0.0f, 22.0f},  
			{10.0f, 0.0f, 22.0f},  
			{0.0f, 10.0f, 22.0f},  
			{10.0f, 10.0f, 22.0f},
			//Back
			{10.0f, 0.0f, 12.0f},  
			{10.0f, 0.0f, 22.0f},  
			{10.0f, 10.0f, 12.0f},  
			{10.0f, 10.0f, 22.0f},
			//Top
			{0.0f, 10.0f, 12.0f}, //16 
			{10.0f, 10.0f, 12.0f},  
			{0.0f, 10.0f, 22.0f},  
			{10.0f, 10.0f, 22.0f}, //19
			{3.0, 10, 15.0}, //20
			{7.0, 10, 15.0},  
			{3.0, 10, 19.0},  
			{7.0, 10, 19.0}, //23
			{0.0f,10.0f,15.0f}, //24
			{0.0f,10.0f,19.0f},
			{10.0f,10.0f,15.0f},
			{10.0f,10.0f,19.0f} //27
		};
		std::vector<uint32_t> indices2 = {
			//Floor
			0, 1, 2,
			1, 2, 3,
			//Left
			4, 5, 6,
			5, 6, 7,
			//Right
			8, 9, 10,
			9, 10, 11,
			//Back
			12, 13, 14,
			13, 14, 15,
			//Top
			16, 24, 26,
			16, 26, 17,
			25, 18, 19,
			25, 19, 27,
			24, 25, 22,
			24, 22, 20,
			21, 23, 27,
			21, 26, 27
		};
		//Stanza 3
		std::vector<glm::vec3> vertices3 = {
			//Floor
			{0.0f, 0.0f, 24.0f},  
			{10.0f, 0.0f, 24.0f},  
			{0.0f, 0.0f, 34.0f},  
			{10.0f, 0.0f, 34.0f},
			//Back
			{10.0f, 0.0f, 24.0f},  
			{10.0f, 0.0f, 34.0f},  
			{10.0f, 10.0f, 24.0f},  
			{10.0f, 10.0f, 34.0f},
			//Top
			{0.0f, 10.0f, 24.0f},  
			{10.0f, 10.0f, 24.0f},  
			{0.0f, 10.0f, 34.0f},  
			{10.0f, 10.0f, 34.0f},
		};
		std::vector<uint32_t> indices3 = {
			//Floor
			0, 1, 2,
			1, 2, 3,
			//Back
			4, 5, 6,
			5, 6, 7,
			//Top
			8, 9, 10,
			9, 10, 11
		};

		std::vector<glm::vec3> verticesMirrorL {
			{0.0f, 0.0f, 24.0f},  
			{10.0f, 0.0f, 24.0f},  
			{0.0f, 10.0f, 24.0f},  
			{10.0f, 10.0f, 24.0f},
		};	
		std::vector<glm::vec3> verticesMirrorR {
			{0.0f, 0.0f, 34.0f},  
			{10.0f, 0.0f, 34.0f},  
			{0.0f, 10.0f, 34.0f},  
			{10.0f, 10.0f, 34.0f}
		};	
		std::vector<uint32_t> indicesMirrorL = {
			0, 1, 2,
			1, 2, 3,
		};
		std::vector<uint32_t> indicesMirrorR = {
			//Left
			0, 1, 2,
			1, 2, 3,
		};

		int mainStride = VDRooms.Bindings[0].stride;

		Room1.vertices = std::vector<unsigned char>(vertices1.size() * sizeof(VertexRooms));
		for(int i = 0; i < vertices1.size(); i++) {
			VertexRooms *V_vertex = (VertexRooms *)(&(Room1.vertices[i * mainStride]));
			V_vertex->pos = vertices1[i];	
			V_vertex->room = glm::vec2(0);
		}
		std::vector<glm::vec3> norm1 = {
			{0.0f,1.0f,0.0f},
			{0.0f,0.0f,1.0f},
			{0.0f,0.0f,-1.0f},
			{-1.0f,0.0f,0.0f},
			{0.0f,1.0f,0.0f},
			{0.0f,1.0f,0.0f},
			{0.0f,1.0f,0.0f},
		};
		std::vector<glm::vec3> room1Colors = {
			{1,1,1},
			{1,0,0},
			{0,1,0},
			{1,1,1},
			{1,1,1},
			{1,1,1},
			{1,1,1}
		};
		for(int i=0; i<norm1.size(); i++) {
			for(int j=0; j<4; j++){
				VertexRooms *V_vertex = (VertexRooms *)(&(Room1.vertices[(i*4 + j) * mainStride]));
				V_vertex->norm = norm1[i];	
				V_vertex->color = room1Colors[i];
			}
		}
		Room1.indices = indices1;
		Room1.initMesh(this, &VDRooms);

		std::vector<glm::vec3> norm2 = {
			{0.0f,1.0f,0.0f},
			{0.0f,0.0f,1.0f},
			{0.0f,0.0f,-1.0f},
			{-1.0f,0.0f,0.0f},
			{0.0f,1.0f,0.0f},
			{0.0f,1.0f,0.0f},
			{0.0f,1.0f,0.0f},
		};
		std::vector<glm::vec3> room2Colors = {
			{1,1,1},
			{1,0,0},
			{0,1,0},
			{1,1,1},
			{1,1,1},
			{1,1,1},
			{1,1,1}
		};
		Room2.vertices = std::vector<unsigned char>(vertices2.size() * sizeof(VertexRooms));
		for(int i = 0; i < vertices2.size(); i++) {
			VertexRooms *V_vertex = (VertexRooms *)(&(Room2.vertices[i * mainStride]));
			V_vertex->pos = vertices2[i];	
			V_vertex->room = glm::vec2(1);
		}
		for(int i=0; i<norm2.size(); i++) {
			for(int j=0; j<4; j++){
				VertexRooms *V_vertex = (VertexRooms *)(&(Room2.vertices[(i*4 + j) * mainStride]));
				V_vertex->norm = norm2[i];	
				V_vertex->color = room2Colors[i];
			}
		}
		Room2.indices = indices2;
		Room2.initMesh(this, &VDRooms);

		Room3.vertices = std::vector<unsigned char>(vertices3.size() * sizeof(VertexRooms));
		for(int i = 0; i < vertices3.size(); i++) {
			VertexRooms *V_vertex = (VertexRooms *)(&(Room3.vertices[i * mainStride]));
			V_vertex->pos = vertices3[i];	
			V_vertex->room = glm::vec2(2);
		}
		std::vector<glm::vec3> room3Colors = {
			{1,1,1},				
			{0,0,1},
			{1,1,1}
		};
		std::vector<glm::vec3> norm3 = {
			{0.0f,1.0f,0.0f},
			{-1.0f,0.0f,0.0f},
			{0.0f,-1.0f,0.0f},
		};
		for(int i=0; i<norm3.size(); i++) {
			for(int j=0; j<4; j++){
				VertexRooms *V_vertex = (VertexRooms *)(&(Room3.vertices[(i*4 + j) * mainStride]));
				V_vertex->norm = norm3[i];	
				V_vertex->color = room3Colors[i];
			}
		}
		
		Room3.indices = indices3;
		Room3.initMesh(this, &VDRooms);

		//Objects inside room1
		//Light
		std::vector<glm::vec3> vertices1Obj = {
			{3.0, 10, 3.0},  
			{7.0, 10, 3.0},  
			{3.0, 10, 7.0},  
			{7.0, 10, 7.0},  
		};
		std::vector<uint32_t> indices1Obj = {
			0, 1, 2,
			1, 2, 3,
		};

		Light1.vertices = std::vector<unsigned char>((vertices1Obj.size()) * sizeof(VertexRooms));
		for(int i = 0; i < vertices1Obj.size(); i++) {
			VertexRooms *V_vertex = (VertexRooms *)(&(Light1.vertices[i * mainStride]));
			V_vertex->pos = vertices1Obj[i];	
			V_vertex->norm = glm::vec3(0.0f, 1.0f, 0.0f);	
			V_vertex->room = glm::vec2(0);
			V_vertex->color = glm::vec3{2,2,2}; //special color indicating the light box
		}
		Light1.indices = indices1Obj;
		Light1.initMesh(this, &VDRooms);

		//Objects inside room2
		//Light
		std::vector<glm::vec3> vertices2Obj = {
			{3.0, 10, 15.0},  
			{7.0, 10, 15.0},  
			{3.0, 10, 19.0},  
			{7.0, 10, 19.0},  
		};
		std::vector<uint32_t> indices2Obj = {
			0, 1, 2,
			1, 2, 3,
		};
		Light2.vertices = std::vector<unsigned char>((vertices2Obj.size() * sizeof(VertexRooms)));
		for(int i = 0; i < vertices2Obj.size(); i++) {
			VertexRooms *V_vertex = (VertexRooms *)(&(Light2.vertices[i * mainStride]));
			V_vertex->pos = vertices2Obj[i];	
			V_vertex->room = glm::vec2(1);
			V_vertex->norm = glm::vec3(0.0f, 1.0f, 0.0f);	
			V_vertex->color = glm::vec3{2,2,2}; //special color indicating the light box
		}
		Light2.indices = indices2Obj;
		Light2.initMesh(this, &VDRooms);

		MirrorL.vertices = std::vector<unsigned char>((verticesMirrorL.size() * sizeof(VertexMirrors) * 2));
		for(int i = 0; i < verticesMirrorL.size(); i++){
			VertexMirrors *V_vertex = (VertexMirrors *)(&(MirrorL.vertices[i * mainStride]));
			V_vertex->pos = verticesMirrorL[i];	
			V_vertex->UV = glm::vec2(verticesMirrorL[i].x / 10.0f, verticesMirrorL[i].y / 10.0f);	
		}
		MirrorL.indices = indicesMirrorL;
		MirrorL.initMesh(this, &VDRooms);

		MirrorR.vertices = std::vector<unsigned char>((verticesMirrorR.size() * sizeof(VertexMirrors) * 2));
		for(int i = 0; i < verticesMirrorR.size(); i++){
			VertexMirrors *V_vertex = (VertexMirrors *)(&(MirrorR.vertices[i * mainStride]));
			V_vertex->pos = verticesMirrorR[i];	
			V_vertex->UV = glm::vec2(verticesMirrorR[i].x / 10.0f, verticesMirrorR[i].y / 10.0f);	
		}
		MirrorR.indices = indicesMirrorR;
		MirrorR.initMesh(this, &VDRooms);

		////////////////////////////////////////////////////////////////////////////////
		//									Spheres									  //
		////////////////////////////////////////////////////////////////////////////////

		for(int i = 0; i < n_objects; i++) {
			S[i].init(this, &VDSpheres, "models/Sphere.obj", OBJ);
		}
	}

	/* Main application parameters */
	void setWindowParameters() {
		windowWidth = 1024;
		windowHeight = 512;
		windowTitle = "GraphicsProject";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f,0.0f,0.0f,1.0f};

		Ar = (float)windowWidth / (float)windowHeight;
	}

	/* Window resize */
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
	}

	/* Load and setup all your Vulkan Models and Texutures. Create your Descriptor set layouts and load the shaders for the pipelines */
	void localInit() {
		///////////	  DSL GP init	///////////
		DSLSphereTransform.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(TransformUniformBufferObject), 1}, //WVP matrix with translation for the specific sphere
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			});
		DSLGlobalTransform.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(TransformUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1} //For the mirrors
			});
		DSLlight.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(LightUniformBufferObject), 1},
			});

		///////////	 DSL Ray init	///////////
		DSLglobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1} //Previous frame texture
			});
		DSLray.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject), 1},
			});


		///////////	  VD GP init	///////////
		VDRooms.init(this, {
				  {0, sizeof(VertexRooms), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
			  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexRooms, pos), sizeof(glm::vec3), POSITION},
			  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexRooms, norm), sizeof(glm::vec3), NORMAL},
			  {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexRooms, color), sizeof(glm::vec3), COLOR},
			  {0, 3, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexRooms, room), sizeof(glm::vec2), UV}
			});

		VDSpheres.init(this, {
				  {0, sizeof(VertexSpheres), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
			  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSpheres, pos), sizeof(glm::vec3), POSITION},
			  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSpheres, norm), sizeof(glm::vec3), NORMAL},
			  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexSpheres, UV), sizeof(glm::vec2), UV}
			});

		VDMirrors.init(this, {
				  {0, sizeof(VertexRooms), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
			  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexMirrors, pos), sizeof(glm::vec3), POSITION},
			  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexMirrors, UV), sizeof(glm::vec3), UV}
			});
		
		///////////	 VD Ray init	///////////
		VD.init(this, {
				  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
			  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION} //ci serve solo questo in teoria per il full screen quad
			});


		///////////	  Pipeline init	  ///////////
		Prooms.init(this, &VDRooms, "shaders/RoomShaderVert.spv", "shaders/RoomShaderFrag.spv", { &DSLGlobalTransform, &DSLlight });
		Prooms.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere1.init(this, &VDSpheres, "shaders/SphereShader1Vert.spv", "shaders/SphereShader1Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere1.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere2.init(this, &VDSpheres, "shaders/SphereShader2Vert.spv", "shaders/SphereShader2Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere2.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere3.init(this, &VDSpheres, "shaders/SphereShader3Vert.spv", "shaders/SphereShader3Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere3.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere4.init(this, &VDSpheres, "shaders/SphereShader4Vert.spv", "shaders/SphereShader4Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere4.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere5.init(this, &VDSpheres, "shaders/SphereShader5Vert.spv", "shaders/SphereShader5Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere5.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere6.init(this, &VDSpheres, "shaders/SphereShader6Vert.spv", "shaders/SphereShader6Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere6.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Psphere7.init(this, &VDSpheres, "shaders/SphereShader7Vert.spv", "shaders/SphereShader7Frag.spv", { &DSLSphereTransform, &DSLlight });
		Psphere7.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Pmirrors.init(this, &VDMirrors, "shaders/MirrorsShaderVert.spv", "shaders/MirrorsShaderFrag.spv", { &DSLGlobalTransform, &DSLlight });
		Pmirrors.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		
		Pray.init(this, &VD, "shaders/RayShaderVert.spv", "shaders/RayShaderFrag.spv", { &DSLglobal, &DSLray });
		Pray.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);


		///////////	  Models init	///////////
		initModels();

		//cover the screen with 2 triangles
		std::vector<Vertex> quadVertices = {
			{{-1.0f,  1.0f, 0.0f}},  // Top-left
			{{-1.0f, -1.0f, 0.0f}},  // Bottom-left
			{{ 1.0f, -1.0f, 0.0f}},  // Bottom-right
			{{ 1.0f,  1.0f, 0.0f}}   // Top-right
		};
		std::vector<uint32_t> indices = {
			0, 1, 2,  // First triangle
			0, 2, 3   // Second triangle
		};

		Mtri.vertices = std::vector<unsigned char>(quadVertices.size() * sizeof(Vertex));
		memcpy(Mtri.vertices.data(), quadVertices.data(), Mtri.vertices.size());

		Mtri.indices = indices;
		Mtri.initMesh(this, &VD);

		
		///////////	  Textures init	  ///////////
		for(int i = 0; i < n_objects; i++) {
			const char* path = ("textures/sphere" + std::to_string(i+1) + ".jpg").c_str();
			T[i].init(this, path);
		}
		TM.init(this, "textures/Mirror.png");

		Timage.init(this, "textures/background.png");
		

		///////////	  Translation mat for spheres  ///////////
		Tpre[0] = glm::translate(glm::mat4(1.0f),glm::vec3(7.0f, 1.5f, 2.5f));
		Tpre[1] = glm::translate(glm::mat4(1.0f),glm::vec3(7.0f, 1.5f, 7.5f));
		Tpre[2] = glm::translate(glm::mat4(1.0f),glm::vec3(7.0f, 1.5f, 14.5f));
		Tpre[3] = glm::translate(glm::mat4(1.0f),glm::vec3(7.0f, 1.5f, 19.5f));
		Tpre[4] = glm::translate(glm::mat4(1.0f),glm::vec3(3.0f, 1.5f, 17.0f));
		Tpre[5] = glm::translate(glm::mat4(1.0f),glm::vec3(5.0f, 7.0f, 29.0f));
		Tpre[6] = glm::translate(glm::mat4(1.0f),glm::vec3(5.0f, 2.64f, 29.0f));


		// Descriptor pool sizes
		// WARNING!!!!!!!!
		// Must be set before initializing the text and the scene
		DPSZs.uniformBlocksInPool = 2 + n_objects*2 + 2; //2.1.2
		DPSZs.texturesInPool = n_objects*2 + 1 + 1;
		DPSZs.setsInPool = 2 + n_objects*2 + 2;


		std::cout << "Initializing text\n";

		std::cout << "Initialization completed!\n";
		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
	}

	/* Create pipelines and Descriptor Sets */
	void pipelinesAndDescriptorSetsInit() {
		// Pipeline 
		Prooms.create();
		Psphere1.create();
		Psphere2.create();
		Psphere3.create();
		Psphere4.create();
		Psphere5.create();
		Psphere6.create();
		Psphere7.create();
		Pmirrors.create();

		Pray.create();


		// Define the data set
		DSLight.init(this, &DSLlight, { });
		DSGlobalGP.init(this, &DSLGlobalTransform, { &TM });
		for(int i = 0; i < n_objects; i++) {
			DSSphere[i].init(this, &DSLSphereTransform, {&T[i]});
		}

		DSray.init(this, &DSLray, { });
		DSGlobal.init(this, &DSLglobal, { &Timage });
	}

	Texture getImage() {
		return Timage;
	}

	/* Destroy pipelines and Descriptor Sets */
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		Prooms.cleanup();
		Psphere1.cleanup();
		Psphere2.cleanup();
		Psphere3.cleanup();
		Psphere4.cleanup();
		Psphere5.cleanup();
		Psphere6.cleanup();
		Psphere7.cleanup();
		Pmirrors.cleanup();

		Pray.cleanup();

		// Cleanup Descriptor Sets
		DSLight.cleanup();
		DSGlobalGP.cleanup();
		for(int i = 0; i < n_objects; i++) {
			DSSphere[i].cleanup();
		}

		DSray.cleanup();
		DSGlobal.cleanup();
	}

	/* Here you destroy all the Models, Texture, Desc. Set Layouts and Pipelines */
	void localCleanup() {
		// Cleanup Models and Textures
		Room1.cleanup();
		Room2.cleanup();
		Room3.cleanup();
		Light1.cleanup();
		Light2.cleanup();
		for(int i = 0; i < n_objects; i++) {
			S[i].cleanup();
			T[i].cleanup();
		}
		MirrorL.cleanup();
		MirrorR.cleanup();
		TM.cleanup();

		Mtri.cleanup();
		Timage.cleanup();
		
		// Cleanup Descriptor Set Layouts
		DSLlight.cleanup();
		DSLGlobalTransform.cleanup();
		DSLSphereTransform.cleanup();

		DSLray.cleanup();
		DSLglobal.cleanup();

		// Destroy Pipelines
		Prooms.destroy();
		Psphere1.destroy();
		Psphere2.destroy();
		Psphere3.destroy();
		Psphere4.destroy();
		Psphere5.destroy();
		Psphere6.destroy();
		Psphere7.destroy();
		Pmirrors.destroy();

		Pray.destroy();
	}


	/* Creation of the command buffer: send to the GPU all the objects you want to draw, with their buffers and textures */
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		/* for each object:
			- bind the pipeline
			- bind the models
			- bind the descriptor sets
			- draw call
		*/	
		Prooms.bind(commandBuffer);
		DSGlobalGP.bind(commandBuffer, Prooms, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Prooms, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		Room1.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Room1.indices.size()), 1, 0, 0, 0);
		Room2.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Room2.indices.size()), 1, 0, 0, 0);
		Room3.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Room3.indices.size()), 1, 0, 0, 0);
		Light1.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Light1.indices.size()), 1, 0, 0, 0);
		Light2.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Light2.indices.size()), 1, 0, 0, 0);
		
		Psphere1.bind(commandBuffer);
		DSSphere[0].bind(commandBuffer, Psphere1, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere1, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[0].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[0].indices.size()), 1, 0, 0, 0);

		Psphere2.bind(commandBuffer);
		DSSphere[1].bind(commandBuffer, Psphere2, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere2, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[1].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[1].indices.size()), 1, 0, 0, 0);

		Psphere3.bind(commandBuffer);
		DSSphere[2].bind(commandBuffer, Psphere3, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere3, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[2].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[2].indices.size()), 1, 0, 0, 0);

		Psphere4.bind(commandBuffer);
		DSSphere[3].bind(commandBuffer, Psphere4, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere4, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[3].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[3].indices.size()), 1, 0, 0, 0);

		Psphere5.bind(commandBuffer);
		DSSphere[4].bind(commandBuffer, Psphere5, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere5, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[4].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[4].indices.size()), 1, 0, 0, 0);

		Psphere6.bind(commandBuffer);
		DSSphere[5].bind(commandBuffer, Psphere6, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere6, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[5].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[5].indices.size()), 1, 0, 0, 0);

		Psphere7.bind(commandBuffer);
		DSSphere[6].bind(commandBuffer, Psphere7, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Psphere7, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		S[6].bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(S[6].indices.size()), 1, 0, 0, 0);

		Pmirrors.bind(commandBuffer);
		DSGlobalGP.bind(commandBuffer, Pmirrors, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSLight.bind(commandBuffer, Pmirrors, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		MirrorL.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MirrorL.indices.size()), 1, 0, 0, 0);
		MirrorR.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MirrorR.indices.size()), 1, 0, 0, 0);

		Pray.bind(commandBuffer);
		Mtri.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, Pray, 0, currentImage);	// The Global Descriptor Set (Set 0)
		DSray.bind(commandBuffer, Pray, 1, currentImage);	// The Material and Position Descriptor Set (Set 1)
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mtri.indices.size()), 1, 0, 0, 0);
	}


	/* Update uniforms */
	void updateUniformBuffer(uint32_t currentImage) {
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);

		//////////// Reset frame buffer ////////////
		if (m != glm::vec3(0.0f, 0.0f, 0.f) || r != glm::vec3(0.0f, 0.0f, 0.f)) {
			numberOfSamples = 0; //in questo caso ci serve = 0 almeno nella shader la media pesata non considera il previous frame (perchè ci stiamo muovendo)
		}

		static float autoTime = true;
		static float cTime = 0.0;
		const float turnTime = 36.0f;
		const float angTurnTimeFact = 2.0f * M_PI / turnTime;

		if (autoTime) {
			cTime = cTime + deltaT;
			cTime = (cTime > turnTime) ? (cTime - turnTime) : cTime;
		}
		cTime += r.z * angTurnTimeFact * 4.0;

		const float ROT_SPEED = glm::radians(120.0f);
		const float MOVE_SPEED = 2.0f;

		CamAlpha = CamAlpha - ROT_SPEED * deltaT * r.y;
		CamBeta = CamBeta - ROT_SPEED * deltaT * r.x;
		CamBeta = CamBeta < glm::radians(-90.0f) ? glm::radians(-90.0f) :
			(CamBeta > glm::radians(90.0f) ? glm::radians(90.0f) : CamBeta);

		glm::vec3 ux = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(1, 0, 0, 1);
		glm::vec3 uz = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 1, 1);
		CamPos = CamPos + MOVE_SPEED * m.x * ux * deltaT;
		CamPos = CamPos + MOVE_SPEED * m.y * glm::vec3(0, 1, 0) * deltaT;
		CamPos = CamPos + MOVE_SPEED * m.z * uz * deltaT;

		static float subpassTimer = 0.0;


		// Standard procedure to quit when the ESC key is pressed
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			if (!pressed) {
				pressed = true;
				counter = (counter+1) % 6;
				currentBox = nextBox(counter);
				numberOfSamples = 0;
			}
		}
		else {
			if (pressed) {
				pressed = false;
			}
		}


		// Here is where you actually update your uniforms				
		glm::mat4 M = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 50.0f);
		M[1][1] *= -1;

		glm::mat4 Mv = glm::rotate(glm::mat4(1.0), -CamBeta, glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0), -CamAlpha, glm::vec3(0, 1, 0)) *
			glm::translate(glm::mat4(1.0), -CamPos);

		glm::mat4 ViewPrj = M * Mv;
		glm::mat4 baseTr = glm::mat4(1.0f);

		// updates global uniforms
		// Light uniform
		LightUniformBufferObject lubo{};
		
		lubo.lightPos[0].v = glm::vec3(5.0f,11.0f,5.0f);
		lubo.lightPos[1].v = glm::vec3(5.0f,11.0f,17.0f);
		lubo.lightPos[2].v = glm::vec3(5.0f, 7.0f, 29.0f);

		lubo.lightDir[0].v = glm::vec3(0.0f,1.0f,0.0f);
		lubo.lightDir[1].v = glm::vec3(0.0f,1.0f,0.0f);
		lubo.lightDir[2].v = glm::vec3(0.0f,1.0f,0.0f);

		lubo.lightColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
		lubo.eyePos = CamPos;
		lubo.currRoom = currentBox-3; 
		DSLight.map(currentImage, &lubo, 0);


		// GP objects
		TransformUniformBufferObject tubo{};
		tubo.mMat = baseTr;
		tubo.mvpMat = ViewPrj * tubo.mMat;
		tubo.nMat = glm::inverse(glm::transpose(tubo.mMat));
		DSGlobalGP.map(currentImage, &tubo, 0);

		TransformUniformBufferObject subo{};
		for(int i = 0; i < n_objects; i++) {
			subo.mMat = baseTr * Tpre[i];
			subo.mvpMat = ViewPrj * subo.mMat;
			subo.nMat = glm::inverse(glm::transpose(subo.mMat));
			DSSphere[i].map(currentImage, &subo, 0);
		}

		// Ray samples
		GlobalUniformBufferObject gubo{};
		gubo.numberOfSamples = numberOfSamples;
		
		DSGlobal.map(currentImage, &gubo, 0);
		numberOfSamples += 1;

		// Camera info
		UniformBufferObject ubo{};
		ubo.cameraPos = CamPos;
		ubo.invViewMatrix = glm::inverse(Mv);
		ubo.invProjectionMatrix = glm::inverse(M);
		ubo.currBox = currentBox;

		DSray.map(currentImage, &ubo, 0);
	}
};

// This is the main: probably you do not need to touch this!
int main() {
	App app;

	try {
		app.run();
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}