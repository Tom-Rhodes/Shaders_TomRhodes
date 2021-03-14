#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

struct Vertex {
	// Position
	glm::vec3 Position;
	// Normal
	glm::vec3 Normal;
	// TexCoords
	glm::vec2 TexCoords;
	// Tangent
	glm::vec3 Tangent;
	// Bitangent
	glm::vec3 Bitangent;
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

class Mesh {
public:
	//  Mesh Data
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;
	unsigned int VAO;

	/*  Functions  */
	// Constructor
	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		// Set up the vertex buffers and its attribute pointers.
		setupMesh();
	}

	// render the mesh
	void Draw(Shader shader)
	{
		// bind appropriate textures
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		unsigned int normalNr = 1;
		unsigned int heightNr = 1;
		//Iterates through all of the textures in the vector
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			//Selects the current texture for subsequent calls
			glActiveTexture(GL_TEXTURE0 + i);
			// Retrieve texture number
			string number;
			string name = textures[i].type;
			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++); // transfer unsigned int to stream
			else if (name == "texture_normal")
				number = std::to_string(normalNr++);
			else if (name == "texture_height")
				number = std::to_string(heightNr++);

			// Send the texture name + number to the shader
			glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
			// Bind the texture to GL_TEXTURE_2D
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}

		// Bind the VAO, draw the elements, and then unbind the VAO
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

private:
	unsigned int VBO, EBO;

	// Further detail on these processes in main.cpp
	void setupMesh()
	{
		// Generate the a single VAO at "VAO"
		glGenVertexArrays(1, &VAO);
		// Generate the buffer object name in the VBO and the EBO
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		//Bind the VAO
		glBindVertexArray(VAO);
		// Bind the VBO into the array buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// Create a data store for the array buffer with the same size as the vertices and the vertex data type size, and the specified data
		// before stating the drawing method to be used for the buffer
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		//Bind the EBO buffer to the element array buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		//Similair to above, but for the element array buffer
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//Set the vertex attribute pointers
		//Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		//Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		//Texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		//Tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		//Bitangent
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		//Unbind the VAO
		glBindVertexArray(0);
	}
};
#endif