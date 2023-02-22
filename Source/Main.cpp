// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "common/shader.hpp"
#include "common/texture.hpp"
#include "common/controls.hpp"
#include "Particle.hpp"

#include <iostream>

Particle particles[1000];
float t = 0.00001f;

glm::mat4 RemoveRotation(glm::mat4& M) {
	// Macierz jednostkowa:
	glm::mat4 MX(1.0);
	// Skopiowanie tylko ostatniej kolumny = translacja:
	MX[3] = M[3];
	return MX;
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "System czasteczek - dym", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Dark blue background
	glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

	// Enable depth test
	//glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("shader.vertexshader", "shader.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint texalphaID = glGetUniformLocation(programID, "texalpha");

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View = glm::lookAt(
		glm::vec3(4, 3, 3), // Camera is at (4,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

	GLuint Texture[2];
	Texture[0] = loadPNG("floor.PNG");
	Texture[1] = loadPNG("smoke.PNG");

	int delay = 0;
	int activeParticle = 0;

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	static const GLfloat vertex_sprite[] = {
		-5.0f, 5.0f, 0.0f,
		5.0f, 5.0f, 0.0f,
		-5.0f, -5.0f, 0.0f,

		-5.0f, -5.0f, 0.0f,
		5.0f, 5.0f, 0.0f,
		5.0f, -5.0f, 0.0f,
	};

	// Two UV coordinatesfor each vertex. They were created with Blender.
	static const GLfloat uv_sprite[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,

		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
	};

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_sprite), vertex_sprite, GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_sprite), uv_sprite, GL_STATIC_DRAW);

	do {
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

		computeMatricesFromInputs();
		Projection = getProjectionMatrix();
		View = getViewMatrix();
		Model = glm::mat4(1.0);
		MVP = Projection * View * Model;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Use our shader
		glUseProgram(programID);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			2,                                // size : U+V => 2
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture[0]);
		glUniform1i(TextureID, 0);

		Model = glm::scale(glm::mat4(1.0f), glm::vec3(50, 50, 50));
		Model = glm::rotate(Model, 1.57f, glm::vec3(1, 0, 0));
		MVP = Projection * View * Model;
		glUniform1f(texalphaID, 1.0f);
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture[1]);
		glUniform1i(TextureID, 1);

		if (!particles[999].active)
			delay++;
		if (!particles[999].active && delay >= 4) {
			delay = 0;
			particles[activeParticle].activate();
			activeParticle++;
		}

		for (int i = 0; i < 1000; i++) {
			float last_x = particles[i].x;
			float last_y = particles[i].y;
			float last_z = particles[i].z;
			Model = glm::translate(glm::mat4(1.0f), glm::vec3(last_x, last_y, last_z));
			Model = glm::translate(Model, glm::vec3(particles[i].x, particles[i].y, particles[i].z));
			MVP = Projection * View * Model;
			MVP = RemoveRotation(MVP);
			if (particles[i].life > 1.0)
				glUniform1f(texalphaID, (1.2 - particles[i].life) * 5);
			else
				glUniform1f(texalphaID, particles[i].life);
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			if (particles[i].life > 0.0f && particles[i].active) {
				glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
				particles[i].live(t);
			}
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	//glDeleteBuffers(1, &alphabuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture[2]);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}