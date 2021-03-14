#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
	unsigned int ID;
	// Constructor for the shader with the vertex and fragment shader paths
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
	{
		
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;
		std::ifstream vFile;
		std::ifstream fFile;
		std::ifstream gFile;
		// Ensures ifstream objects can throw exceptions
		vFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			//Open the shaders
			vFile.open(vertexPath);
			fFile.open(fragmentPath);
			std::stringstream vStream, fStream;
			// Store data in the read buffers in the stringstreams
			vStream << vFile.rdbuf();
			fStream << fFile.rdbuf();
			//Close the files
			vFile.close();
			fFile.close();
			// Convert the data in the stringstream to strings
			vertexCode = vStream.str();
			fragmentCode = fStream.str();
			// If geometry shader path is present, also load the geometry shader
			if (geometryPath != nullptr)
			{
				gFile.open(geometryPath);
				std::stringstream gStream;
				gStream << gFile.rdbuf();
				gFile.close();
				geometryCode = gStream.str();
			}
		}
		//Error catch
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		//Convert the strings to a char pointer
		const char* vCode = vertexCode.c_str();
		const char * fCode = fragmentCode.c_str();

		unsigned int vertex, fragment;
		//Create a shader of type GL_VERTEX_SHADER
		vertex = glCreateShader(GL_VERTEX_SHADER);
		//Set the shaders source: the first param is the shader itself, the second is the number of elements, the third the array of pointers,
		//and the final param is if an array of string lengths, which if null each string is assumed to be null terminated
		glShaderSource(vertex, 1, &vCode, NULL);
		//Compile the specified shader
		glCompileShader(vertex);
		//Function to check for shader compilation erros
		checkCompileErrors(vertex, "VERTEX");
		//Create a shader of type GL_FRAGMENT_SHADER, and follow the above steps for the fragment code
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// If geometry shader is given, compile geometry shader
		unsigned int geometry;
		if (geometryPath != nullptr)
		{
			const char * gShaderCode = geometryCode.c_str();
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &gShaderCode, NULL);
			glCompileShader(geometry);
			checkCompileErrors(geometry, "GEOMETRY");
		}
		// Create a shader program
		ID = glCreateProgram();
		//Attach the two above shaders to the specified program
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		//If there is a geometry shader, also attach that
		if (geometryPath != nullptr)
		glAttachShader(ID, geometry);
		//Links the program object. Any shader objects attached are then created as executables to run on their respective processors.
		glLinkProgram(ID);		//
		checkCompileErrors(ID, "PROGRAM");
		//Delete the shaders now they've been linked to the program
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		if (geometryPath != nullptr)
			glDeleteShader(geometry);

	}
	//Function active the shader
	void use()
	{
		glUseProgram(ID);
	}
	//Functions to set uniforms in the shader
	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}

	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}

	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}

	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string &name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}

	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string &name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}

	void setVec4(const std::string &name, const glm::vec4 &value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string &name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}

	void setMat2(const std::string &name, const glm::mat2 &mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	void setMat3(const std::string &name, const glm::mat3 &mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	void setMat4(const std::string &name, const glm::mat4 &mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	// Function to check for any compilation errors
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM") //If it's a shader
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else // If it's a program
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
#endif