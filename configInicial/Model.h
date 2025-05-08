#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SOIL2/SOIL2.h" // Asegúrate de que SOIL2 esté correctamente configurado
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"  // Mesh.h debe definir Vertex y Texture structs
#include "Shader.h"

// Declaración anticipada de la función TextureFromFile para que Model pueda usarla
GLuint TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

class Model
{
public:
    std::vector<Texture> textures_loaded; // Moverlo a public para debug si es necesario, o mantener privado
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    // Constructor, espera una ruta a un modelo 3D.
    Model(const std::string& path, bool gamma = false) : gammaCorrection(gamma)
    {
        this->loadModel(path);
    }

    // Dibuja el modelo, y por lo tanto todas sus mallas
    void Draw(Shader& shader) // Pasar Shader por referencia
    {
        for (GLuint i = 0; i < this->meshes.size(); i++)
        {
            this->meshes[i].Draw(shader);
        }
    }

private:
    // Carga un modelo con extensiones soportadas por ASSIMP desde un archivo y almacena las mallas resultantes en el vector meshes.
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals); // Añadido GenSmoothNormals

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        this->directory = path.substr(0, path.find_last_of('/'));
        if (this->directory == path) { // Si no se encontró '/', el archivo está en el directorio actual
            this->directory = ".";
        }


        this->processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (GLuint i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            this->meshes.push_back(this->processMesh(mesh, scene));
        }

        for (GLuint i = 0; i < node->mNumChildren; i++)
        {
            this->processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        for (GLuint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        for (GLuint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (GLuint j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // 1. Texturas Difusas
            std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            // 2. Texturas Especulares
            std::vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            // (Opcional) Podrías cargar mapas normales aquí también si los usas:
            // std::vector<Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal"); // Assimp a veces carga mapas normales como mapas de altura
            // textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // std::vector<Texture> normalMapsTangent = this->loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
            // textures.insert(textures.end(), normalMapsTangent.begin(), normalMapsTangent.end());
        }
        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<Texture> textures;
        for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            GLboolean skip = false;
            for (GLuint j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.C_Str(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                Texture texture;
                // Pasar this->gammaCorrection a TextureFromFile
                texture.id = TextureFromFile(str.C_Str(), this->directory, this->gammaCorrection);
                if (texture.id != 0) // Solo añadir si la textura se cargó correctamente
                {
                    texture.type = typeName;
                    texture.path = str;
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
        }
        return textures;
    }
};

// Definición de la función TextureFromFile (movida desde el final para mejor organización o puede estar en Model.cpp)
GLuint TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = std::string(path);
    if (filename.rfind(directory, 0) != 0 && (filename.find(":/") == std::string::npos && filename.find(":\\") == std::string::npos)) {
        if (directory != "." && !directory.empty()) {
            filename = directory + '/' + filename;
        }
    }


    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // SOIL_LOAD_AUTO intentará determinar el número de canales
    unsigned char* data = SOIL_load_image(filename.c_str(), &width, &height, &nrComponents, SOIL_LOAD_AUTO);
    if (data)
    {
        GLenum format;
        GLenum internalFormat;
        if (nrComponents == 1)
        {
            format = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            format = GL_RGB;
            internalFormat = gamma ? GL_SRGB : GL_RGB; // Corregir gamma si es necesario
        }
        else if (nrComponents == 4)
        {
            format = GL_RGBA;
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA; // Corregir gamma si es necesario
        }
        else // Formato no soportado o error
        {
            std::cerr << "TextureFromFile: Error, número de componentes no soportado (" << nrComponents << ") para la textura " << filename << std::endl;
            SOIL_free_image_data(data);
            glDeleteTextures(1, &textureID);
            return 0; // Retornar 0 para indicar fallo
        }


        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SOIL_free_image_data(data);
    }
    else
    {
        std::cerr << "TextureFromFile: Error al cargar la textura '" << filename << "'. SOIL2 Error: " << SOIL_last_result() << std::endl;
        glDeleteTextures(1, &textureID); // Limpiar la textura generada si la carga falla
        return 0; // Retornar 0 para indicar fallo
    }

    return textureID;
}