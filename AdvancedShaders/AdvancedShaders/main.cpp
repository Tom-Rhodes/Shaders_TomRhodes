#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void renderQuad();

//Paths for each of the maps used for the wall
char const * diffuse = ("textures/bricks2.jpg");
char const * normal = ("textures/bricks2_normal.jpg");
char const * displacement = ("textures/bricks2_disp.jpg");

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
float heightScale = 0.1;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
	// Initiates the GLFW library
	glfwInit();
	//Specifies the GLFW version (MAJOR.MINOR.0 = 3.3.0)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//Specifies the core GLFW profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creates a window pointer with set dimensions and a name, along with an error catch
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Advanced Shaders", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	//Sets the context of the window current on the thread
	glfwMakeContextCurrent(window);
	//Sets a callback function to accomodate resizing
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//Sets a callback function for the mouse and keyboard input
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Tells GLFW to take cursor input without showing the cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Pass the function to load the address of the OpenGL function pointers,
	// using glfwGetProcAddress that defines the correct fucntion for our OS, with an error catch
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	//Enables depth testing, which uses the depth buffer to compare the depth (z) values of fragements
	//to see if they lie behind other fragments.
	glEnable(GL_DEPTH_TEST);

	// Creates a shader using the specified files
	Shader shader("shaders/vert.vs", "shaders/frag.fs");

	//Loads the maps from the paths provided, and stores their texture ids.
	unsigned int diffuseMap = loadTexture(diffuse);
	unsigned int normalMap = loadTexture(normal);
	unsigned int heightMap = loadTexture(displacement);

	 // Call glUseProgram on the shader
	shader.use();
	//Sets a uniform of the active shader program, passing through the name of the uniform and the value
	//Here it's setting ints for the maps
	shader.setInt("diffuseMap", 0);
	shader.setInt("normalMap", 1);
	shader.setInt("depthMap", 2);

	// The light position
	glm::vec3 lightPos(0.5f, 1.0f, 0.3f);

	// Render loop while the glfwWindow is still open
	while (!glfwWindowShouldClose(window))
	{
		//Sorts frame rate
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Function to handle input for the window
		processInput(window);

		// Clears the colour to a dark grey
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//Clears the colour buffer to the glClearColor and clears the depth buffer to prevent carrying over
		// of data from the previous frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Creates a new 4x4 perspective matrix with an fov, the screen ration, and the near/far clipping planes
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		//Returns the camera's view matrix and stores it
		glm::mat4 view = camera.GetViewMatrix();

		shader.use();
		//Setting 4x4 matrices in the shaders for the projection and view
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		//Create a quad
		glm::mat4 model = glm::mat4(1.0f);
		//Rotatest the model
		model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
		//Setting the model, the position of the camera, the light posiiton, and the height scale to the shader
		shader.setMat4("model", model);
		shader.setVec3("viewPos", camera.Position);
		shader.setVec3("lightPos", lightPos);
		shader.setFloat("heightScale", heightScale);
		//Prints the current height scale, which can be altered by using Q and E
		std::cout << heightScale << std::endl;
		//Sets the texture to be effected by following references
		glActiveTexture(GL_TEXTURE0);
		//Bind the texture as a 2D texture with the diffuseMap, normalMap, and heightMap
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, heightMap);
		//Renders the quad
		renderQuad();

		// Swaps between the currently displayed buffer and the buffer being drawn to
		glfwSwapBuffers(window);
		//Checks for input/events and calls the appropriate callback function
		glfwPollEvents();
	}
	//Cleans and deletes all the allocated GLFW resources
	glfwTerminate();
	return 0;
}

// Function to render a 1x1 quad
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		// Sets the coord positions of the corners
		glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
		glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
		glm::vec3 pos3(1.0f, -1.0f, 0.0f);
		glm::vec3 pos4(1.0f, 1.0f, 0.0f);
		// Sets the texture coordinates
		glm::vec2 uv1(0.0f, 1.0f);
		glm::vec2 uv2(0.0f, 0.0f);
		glm::vec2 uv3(1.0f, 0.0f);
		glm::vec2 uv4(1.0f, 1.0f);
		// Normal vector
		glm::vec3 nm(0.0f, 0.0f, 1.0f);

		// Calculate tangent/bitangent vectors of both triangles
		glm::vec3 tangent1, bitangent1;
		glm::vec3 tangent2, bitangent2;
		// First triangle
		glm::vec3 edge1 = pos2 - pos1;
		glm::vec3 edge2 = pos3 - pos1;
		glm::vec2 deltaUV1 = uv2 - uv1;
		glm::vec2 deltaUV2 = uv3 - uv1;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent1 = glm::normalize(tangent1);

		bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent1 = glm::normalize(bitangent1);

		// Second triangle
		edge1 = pos3 - pos1;
		edge2 = pos4 - pos1;
		deltaUV1 = uv3 - uv1;
		deltaUV2 = uv4 - uv1;

		f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent2 = glm::normalize(tangent2);


		bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
		bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
		bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		bitangent2 = glm::normalize(bitangent2);


		float quadVertices[] = {
			// Positions            // Normal         // TexCoords  // Tangent                          // Bitangent
			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

			pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
			pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
		};
		// configure plane VAO
		//GenBuffers() creates buffer object names in the specified object.
		//Here is creates one in quadVBO which is initialised above.
		glGenBuffers(1, &quadVBO);

		/*GenVertexArrays() creates a specified number of vertex array object (VAO) names in the specified array.
		BindVertexArray() then binds the VAO with the name in the specified array.
		Here we generate one VAO in quadVAO and then bind quadVAO.*/
		glGenVertexArrays(1, &quadVAO);
		glBindVertexArray(quadVAO);
		
		/*BindBuffer() binds a buffer object to a specific buffer binding point
		Here is binds the VBO created above to the ARRAY_BUFFER binding point.*/
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

		/*BufferData() creates a data store for the buffer object bound to the specified buffer binding point.
		The second parameter states the size the data store needs to be, the third points to the data needed to be stored.
		The final parameter indicates to the GL implementation the expected usage of this data.
		Here it's used to create a data store for the VBO bound above to the ARRAY_BUFFER with the size of the quadVertices array
		Then is states a pointer to the data in the vertices array, and indicates it'll be used GL drawing and image commands.*/
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

		//EnableVertexAttribArray enables the generic vertex attribute at the specifiedd index, which is 0 here.
		glEnableVertexAttribArray(0);

		/*VertexAttribPointer specifies the location and data type of the array of generic vertex attributes stated in the first parameter.
		The next parameter states the number of components per attribute; the third parameter states the data type of each component.
		The fourth parameter is a TRUE or FALSE indicates if the stored values are normalised. The fifth specifies the byte stride between attributes.
		The last parameter specifies an offset of the first component of the first attribute in the data store of the currently bound buffer.
		For this usage, it states that at the 0 index in the generic vertex attribute there are three components per attribute, they are of type gl::Float,
		and they're not normalised. It then states the is a stride of 14 for the size of the float data type, and there is no offset for the first component.
		This function are repeated, with an only the index and the offset of 3 for each increase*/
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	//Draws from the array data with primitive type of GL_TRIANGLEs, starting index of 0, and 6 indicies to be drawn.
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//Unbinds the vertex array
	glBindVertexArray(0);
}

// Deals with the inputs through polling glfw if a key has been pressed
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);	//Close the window on ESC
	//WASD camera mo
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	//If Q/E are pressed, increase/decrease the percieved depth of the shaders
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		if (heightScale > 0.0f)
			heightScale -= 0.0005f;
		else
			heightScale = 0.0f;
	}
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		if (heightScale < 1.0f)
			heightScale += 0.0005f;
		else
			heightScale = 1.0f;
	}
}

//Callback for resizing the window
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// Callback for when the mouse is moved
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//If it's the first time the mouse has moved, set it's position to the centre
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;
	//Sends event to camera to process camera movement
	camera.ProcessMouseMovement(xoffset, yoffset);
}

// SCrolling callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//Sends event to camera to process zooming
	camera.ProcessMouseScroll(yoffset);
}

// Function to load a texture from a file path
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	//GenTextures() generates a specified nunber of texture names in a specified array. This usage creates one texture name in textureID.
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	/*stbi_load() loads an image and stores it as char pointer that points to the pixel data
	The first parameter is the image path, the second and third the dimensions, the fourth the image components per pixel,
	and the last forces a specific number of components*/
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		//Gets the format of the image from the stbi_load()
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		//BindTexture() binds a named texture stated by the second parameter to the target specified by the first parameter.
		//	In this case, it's binding the named texture textureID, generated above, to the TEXTURE_2D target.
		glBindTexture(GL_TEXTURE_2D, textureID);
		/*TexImage2D() specifies a 2D texture image. The first parameter in the function states the target texture.
		The second parameter states the image level; the third parameter the number of colour components in the texture; the fourth parameter the width; the fifth parameter the height.
		The next parameter is the width of the border which has to be 0, and the seventh parameter is the format of the pixel data.
		The eighth parameter states the data type of the pixel data, and then the last parameter points to the image data itself.
		So for this use, it uses the GL_TEXTURE_2D to get the target texture. It indicates the base image level of 0 and the format from above.
		Then it inputs the width and height, then the required value of 0 for the border, followed by format again.
		Finally, it states that data type of the pixel data is gl::UNSIGNED_BYTE and then points to the data attached loaded fromt the stbi_load().*/
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		//Generates a mipmap for the GL_TEXTURE_2D texture object
		glGenerateMipmap(GL_TEXTURE_2D);
		//These specify rules/settingsf for the GL_TEXTURE_2D texture object, such as here where it states it should repeat the texture in either direction
		//if it extends beyond the texture's size, along with the texture filtering for how OpenGL chooses the texture pixel colour from.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//Frees the loaded image
		stbi_image_free(data);
	}
	//Error catch
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}
	//Returns the texture's ID
	return textureID;
}