#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



#include "Shader.h"

using namespace std;

struct Vertex
{
	// Position
	glm::vec3 Position;
	// Normal
	glm::vec3 Normal;
	// TexCoords
	glm::vec2 TexCoords;
};

struct Texture
{
	GLuint id;
	string type;
	aiString path;
};

class Mesh
{
public:
	/*  Mesh Data  */
	vector<Vertex> vertices;
	vector<GLuint> indices;
	vector<Texture> textures;

	/*  Functions  */
	// Constructor
	Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		// Now that we have all the required data, set the vertex buffers and its attribute pointers.
		this->setupMesh();
	}

	// En Mesh::Draw(Shader shader)
	void Draw(Shader shader)
	{
		GLuint diffuseUnit = 0;    // Unidad de textura para difusa
		GLuint specularUnit = 1;   // Unidad de textura para especular
		bool diffuseTextureBound = false;
		bool specularTextureBound = false;

		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			string type = this->textures[i].type; // "texture_diffuse" o "texture_specular"

			if (type == "texture_diffuse" && !diffuseTextureBound)
			{
				glActiveTexture(GL_TEXTURE0 + diffuseUnit);
				glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
				// El uniform "material.diffuse" ya está configurado para usar la unidad 'diffuseUnit' (0)
				// desde IOSLab.cpp, así que no necesitas hacer glUniform1i aquí de nuevo para el sampler.
				diffuseTextureBound = true;
			}
			else if (type == "texture_specular" && !specularTextureBound)
			{
				glActiveTexture(GL_TEXTURE0 + specularUnit);
				glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
				// El uniform "material.specular" ya está configurado para usar la unidad 'specularUnit' (1)
				// desde IOSLab.cpp.
				specularTextureBound = true;
			}
			// Si tienes más de una textura difusa o especular por malla,
			// este código simple solo usará la primera de cada tipo.
			// Si necesitas múltiples, el shader tendría que cambiar (ej. array de samplers o más samplers).
		}

		// Si alguna textura no se encontró/enlazó, puedes enlazar una textura por defecto (opcional)
		// o asegurarte de que tus modelos siempre tengan al menos una textura difusa.
		// Por ejemplo, si no hay textura difusa, podrías enlazar una textura blanca 1x1.

		glUniform1f(glGetUniformLocation(shader.Program, "material.shininess"), 16.0f); // Esto está bien

		// Draw mesh
		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Desvincular texturas (buena práctica)
		if (diffuseTextureBound) {
			glActiveTexture(GL_TEXTURE0 + diffuseUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		if (specularTextureBound) {
			glActiveTexture(GL_TEXTURE0 + specularUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

private:
	/*  Render data  */
	GLuint VAO, VBO, EBO;

	/*  Functions    */
	// Initializes all the buffer objects/arrays
	void setupMesh()
	{
		// Create buffers/arrays
		glGenVertexArrays(1, &this->VAO);
		glGenBuffers(1, &this->VBO);
		glGenBuffers(1, &this->EBO);

		glBindVertexArray(this->VAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		// again translates to 3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);
		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, Normal));
		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
};