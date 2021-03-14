#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"
#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
	vector<Texture> textures_loaded;
	vector<Mesh> meshes;
	string directory;
	bool gammaCorrection;	
	// Fucntion to load the model from the given path
	Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
	{
		loadModel(path);
	}

	// Draw the meshes for the shader passed in
	void Draw(Shader shader)
	{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shader);
	}

private:
	// Loads a model using ASSIMP
	void loadModel(string const &path)
	{
		// Use the ASSIMP importer to read the file data
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// Error check
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// Retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('/'));

		// Process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	// Function to process a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes if there are any present.
	void processNode(aiNode *node, const aiScene *scene)
	{
		// Process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// Node object only contains indices to index the actual objects in the scene, while scene contains all the data.
			// Node is primarily for organisation.
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//Adds the mess the vector
			meshes.push_back(processMesh(mesh, scene));
		}
		// After we've processed all meshes, we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}

	}
	//Function process the mesh
	Mesh processMesh(aiMesh *mesh, const aiScene *scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;
		// Iterate through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;
			//Position
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			//Normals
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
			//Texture coordinates
			if (mesh->mTextureCoords[0]) // Check if the mesh contains texture coordinates
			{
				glm::vec2 vec;
				// As a vertex can contain up to 8 different texture coordinates, we can't assume that we won't 
				// use models where a vertex can have multiple texture coordinates. So we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				//Set the vertex's new texcture coordinates
				vertex.TexCoords = vec;
			}
			else      //If no texture coordinates
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			// Tangent
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.Tangent = vector;
			// Bitangent
			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.Bitangent = vector;
			//Push back the vertex onto the vertices vector
			vertices.push_back(vertex);
		}
		// Iterate through the mesh's faces to get the indicies
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// Store all indicies in the vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// Process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		//Creates a new vector of textures using the passed through material, the texture type, and the type's name
		//Diffuse
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//Specular
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		//Normal
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		// height
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		//Return a mesh object with all the data
		return Mesh(vertices, indices, textures);
	}

	// Checks all material textures of a given type and loads the textures if they're not loaded yet.
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
	{
		vector<Texture> textures;
		//Iterates through the textures of the specified type
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			//Gets the texture and stores it as an aiString
			mat->GetTexture(type, i, &str);
			// Check if texture was loaded before and if so, continue to next iteration
			bool skip = false;
			//For all the loaded textures
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{   // If the texture hasn't been loaded already, load it
				Texture texture;
				//Get the texture from the file, and set it's various values
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);	//Push it back to the textures
				textures_loaded.push_back(texture);  // Push it back to the loaded textures so we don't load it again
			}
		}
		//Return the vector of textures
		return textures;
	}
};

//Explained in greater detail in main.cpp
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
	string filename = string(path);
	//Add the filename to the directory to get the full path
	filename = directory + '/' + filename;

	unsigned int textureID;
	//Generates a single texture name in textureID
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	//Loads the file at the path, storing the dimensions and the colour components
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		//Checks for the loaded texture's format
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		//Bind the texture to GL_TEXTURE_2D
		glBindTexture(GL_TEXTURE_2D, textureID);
		//Specifies a texture 2D image with the target texture first, then the image level, the colour componenets, the width, height,
		//the border width, the pixel data format, the data type of pixel data, and then the image data itself
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		//Creates a mipmap for the GL_TEXTURE_2D texture
		glGenerateMipmap(GL_TEXTURE_2D);
		//Sets the parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//Frees the laoded image
		stbi_image_free(data);
	}
	//Error check
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}
	//Returns the texture's ID
	return textureID;
}
#endif