#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
// Assimp includes no son necesarios aqu� si Model.h los maneja
// #include <assimp/Importer.hpp>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>

#include "Shader.h" // Aseg�rate que Shader.h est� bien

// Definiciones de Vertex y Texture (si no est�n ya en un archivo global de utilidades)
struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture
{
    GLuint id;
    std::string type; // "texture_diffuse", "texture_specular", "texture_normal"
    aiString path;  // Para la optimizaci�n de no cargar duplicados (aiString viene de assimp/material.h)
    // Si quieres evitar la dependencia de Assimp aqu�, podr�as cambiar path a std::string
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    GLuint VAO; // Hacerlo p�blico para debug, o mantener privado

    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<Texture>& textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->setupMesh();
    }

    void Draw(Shader& shader) // Pasar Shader por referencia
    {
        // Unidades de textura fijas para difusa y especular
        // Los samplers en el shader (material.diffuse, material.specular) ya est�n
        // configurados para usar estas unidades desde IOSLab.cpp
        const GLuint diffuseUnit = 0;
        const GLuint specularUnit = 1;
        // const GLuint normalUnit = 2; // Si usaras mapas normales

        unsigned int diffuseNr = 0;  // Para contar cu�ntas texturas difusas hemos enlazado
        unsigned int specularNr = 0; // Para contar cu�ntas texturas especulares hemos enlazado
        // unsigned int normalNr = 0;

        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            // Asumimos que el shader est� activo (shader.Use() se llama antes de Model::Draw)
            if (this->textures[i].id == 0) continue; // Saltar texturas que no se cargaron

            if (this->textures[i].type == "texture_diffuse")
            {
                if (diffuseNr == 0) // Solo enlazar la primera textura difusa a la unidad 0
                {
                    glActiveTexture(GL_TEXTURE0 + diffuseUnit);
                    glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
                }
                diffuseNr++;
            }
            else if (this->textures[i].type == "texture_specular")
            {
                if (specularNr == 0) // Solo enlazar la primera textura especular a la unidad 1
                {
                    glActiveTexture(GL_TEXTURE0 + specularUnit);
                    glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
                }
                specularNr++;
            }
            // else if (this->textures[i].type == "texture_normal")
            // {
            //     if (normalNr == 0) // Solo enlazar la primera textura normal a la unidad 2
            //     {
            //         glActiveTexture(GL_TEXTURE0 + normalUnit);
            //         glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
            //     }
            //     normalNr++;
            // }
        }

        // El uniform material.shininess se establece aqu�
        // Si tienes otros uniforms de material por malla, config�ralos aqu�.
        // shader.setFloat("material.shininess", 16.0f); // Usando un m�todo de Shader si lo tienes
        glUniform1f(glGetUniformLocation(shader.Program, "material.shininess"), 32.0f); // Valor de ejemplo, ajusta seg�n necesites


        // Draw mesh
        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(this->indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Siempre es buena pr�ctica desvincular las texturas despu�s de usarlas,
        // aunque si el siguiente objeto las va a sobreescribir, no es estrictamente necesario.
        if (diffuseNr > 0) {
            glActiveTexture(GL_TEXTURE0 + diffuseUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        if (specularNr > 0) {
            glActiveTexture(GL_TEXTURE0 + specularUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // if (normalNr > 0) {
        //     glActiveTexture(GL_TEXTURE0 + normalUnit);
        //     glBindTexture(GL_TEXTURE_2D, 0);
        // }
    }


private:
    // GLuint VAO, VBO, EBO; // Movido a p�blico para debug o mantener privado
    GLuint VBO, EBO;


    void setupMesh()
    {
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);

        glBindVertexArray(this->VAO);

        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

        // Vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // Vertex Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        // Vertex Texture Coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};