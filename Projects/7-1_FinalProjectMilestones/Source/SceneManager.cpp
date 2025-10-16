///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			//std::cout << "failed to load " << index << std::endl; // so you know when running the app
			index++;
		}
	}

	return(true); // this was return true not return bFound
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/ceramic.jpg",
		"ceramic");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/porcelain.jpg",
		"porcelain");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"metal");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/paper.jpg",
		"paper");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/plastic.jpg",
		"plastic");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/drywall.jpg",
		"drywall");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for preparing the textures properties
 *  the impacts how light behaves on the object surface.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// easier tuning

	float ambC = .1;
	float ambS = .75;
	float difC = .25;
	float specC = .75;
	float shin = 16.0;

	OBJECT_MATERIAL ceramic;
	ceramic.tag = "ceramic";
	ceramic.ambientColor = glm::vec3(ambC); 
	ceramic.ambientStrength = ambS;          
	ceramic.diffuseColor = glm::vec3(difC);
	ceramic.specularColor = glm::vec3(specC); 
	ceramic.shininess = shin;           
	m_objectMaterials.push_back(ceramic);

	OBJECT_MATERIAL porcelain;
	porcelain.tag = "porcelain";
	porcelain.ambientColor = glm::vec3(ambC);
	porcelain.ambientStrength = ambS;
	porcelain.diffuseColor = glm::vec3(difC);
	porcelain.specularColor = glm::vec3(specC);
	porcelain.shininess = shin;
	m_objectMaterials.push_back(porcelain);

	OBJECT_MATERIAL metal;
	metal.tag = "metal";
	metal.ambientColor = glm::vec3(ambC);
	metal.ambientStrength = ambS;
	metal.diffuseColor = glm::vec3(difC);
	metal.specularColor = glm::vec3(specC);
	metal.shininess = shin;
	m_objectMaterials.push_back(metal);

	OBJECT_MATERIAL paper;
	paper.tag = "paper";
	paper.ambientColor = glm::vec3(ambC);
	paper.ambientStrength = ambS;
	paper.diffuseColor = glm::vec3(difC);
	paper.specularColor = glm::vec3(specC);
	paper.shininess = shin;
	m_objectMaterials.push_back(paper);

	OBJECT_MATERIAL plastic;
	plastic.tag = "plastic";
	plastic.ambientColor = glm::vec3(ambC);
	plastic.ambientStrength = ambS;
	plastic.diffuseColor = glm::vec3(difC);
	plastic.specularColor = glm::vec3(specC);
	plastic.shininess = shin;
	m_objectMaterials.push_back(plastic);

	OBJECT_MATERIAL drywall;
	drywall.tag = "drywall";
	drywall.ambientColor = glm::vec3(ambC);
	drywall.ambientStrength = ambS;
	drywall.diffuseColor = glm::vec3(difC);
	drywall.specularColor = glm::vec3(specC);
	drywall.shininess = shin;
	m_objectMaterials.push_back(drywall);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used for defining the scene lights and
 *  the phong lighting parameters in the shader.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	//easier tuning
	const float amb = 0.1f;   
	const float dif = 0.12f;   
	const float H = 6.0f;    
	const float R = 12.0f;   

	//makes lights in the corners of the scene.
	//which ever is last in the array gets the glare
	const glm::vec3 corners[4] = {
		{ R, H, -R}, { R, H,  R}, {-R, H,  R}, {-R, H, -R}
	};

	for (int i = 0; i < 3; ++i) {
		std::string base = "lightSources[" + std::to_string(i) + "]";
		m_pShaderManager->setVec3Value((base + ".position").c_str(), corners[i]);
		m_pShaderManager->setVec3Value((base + ".ambientColor").c_str(), amb, amb, amb);
		m_pShaderManager->setVec3Value((base + ".diffuseColor").c_str(), dif, dif, dif);
		m_pShaderManager->setVec3Value((base + ".specularColor").c_str(), 0.0f, 0.0f, 0.0f);
		m_pShaderManager->setFloatValue((base + ".specularIntensity").c_str(), 0.0f);
		m_pShaderManager->setFloatValue((base + ".focalStrength").c_str(), 3.0f);
	}

	//only light that makes glare aka specular effect
	std::string base = "lightSources[3]";
	m_pShaderManager->setVec3Value((base + ".position").c_str(), corners[3]);
	m_pShaderManager->setVec3Value((base + ".ambientColor").c_str(), amb, amb, amb);
	m_pShaderManager->setVec3Value((base + ".diffuseColor").c_str(), dif, dif, dif);
	m_pShaderManager->setVec3Value((base + ".specularColor").c_str(), 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setFloatValue((base + ".specularIntensity").c_str(), 0.15f);
	m_pShaderManager->setFloatValue((base + ".focalStrength").c_str(), 25.0f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// 1) load and bind textures
	LoadSceneTextures();

	// 2) define materials (even for textured objects)
	DefineObjectMaterials();

	// 3) set up lights and enable lighting
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/

	// plane / floor / ground 

	/******************************************************************/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 20.0f, 20.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	SetShaderTexture( "porcelain");
	SetShaderMaterial("porcelain");
	SetTextureUVScale(8, 8);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/******************************************************************/

	// plate / tappered cylinder 

	/******************************************************************/
	/******************************************************************/
	scaleXYZ = glm::vec3(4.0f, 1.0f, 4.0f);
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.0f, 1.0f, 2.0f);

	SetTransformations( scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 1);
	SetShaderTexture("ceramic");
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
	/******************************************************************/

	// cup / tappered cylinder / torus / plane 

	/******************************************************************/
	// tappered cylinder
	/******************************************************************/
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);
	/* same rotation as above minor computational savings // keep in for readability */
	//XrotationDegrees = 180.0f;
	//YrotationDegrees = 0.0f;
	//ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.0f, 4.10f, 2.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("ceramic");
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
	// torus
	/******************************************************************/
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 120.0f;
	positionXYZ = glm::vec3(-6.5f, 3.0f, 3.75f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("ceramic");
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawHalfTorusMesh();
	/****************************************************************/
	/******************************************************************/

	// book / box / torus

	/******************************************************************/
	// Box
	/******************************************************************/
	scaleXYZ = glm::vec3(6.0f, 0.5f, 11.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.5f, 0.25f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(2, 2);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
	// torus'
	/******************************************************************/
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 0.0f;

	// I used a formula to find the concrete values instead of coding in the trig 
	// 24 x 3 matrix of positions
	float positions[24][3] = { 
		{5.537341f, 0.250000f, -3.064676f}, 
		{5.308174f, 0.250000f, -2.667747f}, 
		{5.079008f, 0.250000f, -2.270819f}, 
		{4.849841f, 0.250000f, -1.873891f}, 
		{4.620674f, 0.250000f, -1.476962f}, 
		{4.391508f, 0.250000f, -1.080034f}, 
		{4.162341f, 0.250000f, -0.683105f}, 
		{3.933174f, 0.250000f, -0.286177f}, 
		{3.704008f, 0.250000f, 0.110752f}, 
		{3.474841f, 0.250000f, 0.507680f}, 
		{3.245674f, 0.250000f, 0.904609f}, 
		{3.016508f, 0.250000f, 1.301537f}, 
		{2.787341f, 0.250000f, 1.698465f}, 
		{2.558174f, 0.250000f, 2.095394f}, 
		{2.329008f, 0.250000f, 2.492322f}, 
		{2.099841f, 0.250000f, 2.889251f}, 
		{1.870674f, 0.250000f, 3.286179f}, 
		{1.641508f, 0.250000f, 3.683108f}, 
		{1.412341f, 0.250000f, 4.080036f}, 
		{1.183174f, 0.250000f, 4.476964f}, 
		{0.954008f, 0.250000f, 4.873893f}, 
		{0.724841f, 0.250000f, 5.270821f}, 
		{0.495674f, 0.250000f, 5.667750f}, 
		{0.266508f, 0.250000f, 6.064678f}, 
	};

	for (int i = 0; i < 24; ++i) {
		glm::vec3 positionXYZ(
			positions[i][0],
			positions[i][1],
			positions[i][2]
		);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		//SetShaderColor(0.0, 0.0, 0.0, 1);
		SetShaderTexture("plastic");
		SetShaderMaterial("plastic");
		m_basicMeshes->DrawTorusMesh();
	}

	/******************************************************************/
	/******************************************************************/

	// pencil / cylinder / cone

	/******************************************************************/
	// pen cylinder metal
	/******************************************************************/
	scaleXYZ = glm::vec3(0.25f, 2.0f, 0.25f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.0f, 5.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("metal");
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	// pen cylinder plastic
	/******************************************************************/
	scaleXYZ = glm::vec3(0.25f, 2.0f, 0.25f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.0f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.0, 0.0, 0.0, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	// pen cone plastic tip
	/******************************************************************/
	scaleXYZ = glm::vec3(0.25f, 1.0f, 0.25f);
	XrotationDegrees = 270.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.0f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.0, 0.0, 0.0, 1);
	SetShaderTexture("plastic");
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
}

