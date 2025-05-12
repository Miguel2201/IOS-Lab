#include <iostream>
#include <cmath>
#include <vector>
#include <string> // Para std::to_string
#include <map>    // Para asociar modelos con sus escalas animadas

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Load Models
// #include "SOIL2/SOIL2.h" // Si está en Model.h

// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 1000;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(25.0f, 5.0f, 6.5f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
bool active; // Para la luz parpadeante (Light1_Color_Factors)

// Positions of the point lights 
glm::vec3 pointLightPositions[] = {
    glm::vec3(-22.75f, 9.25f, 11.0f),
    glm::vec3(-18.75f, 9.25f, 11.0f),
    glm::vec3(-14.75f, 9.25f, 11.0f),
    glm::vec3(-8.75f, 9.25f, 11.0f)
};

// Vértices para el cubo de luz (lámparas)
float lampVertices[] = { 
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
};


glm::vec3 Light1_Color_Factors = glm::vec3(0); // Para controlar el parpadeo de la luz 0

// Deltatime
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// --- Variables para la Animación de Desaparición ---
bool startDisappearAnimation = false;
bool modelsVanished = false;
const float disappearSpeed = 0.7f;
std::vector<Model*> animatableModels; // Vector para los modelos que desaparecen
std::vector<float> modelScales;       // Escalas para los modelos en animatableModels
std::map<Model*, glm::mat4> originalModelTransforms; // Sus transformaciones originales
std::map<Model*, size_t> modelToIndexMap;          // Para encontrar su índice en modelScales
Model* Laboratorio_permanente = nullptr;
// ----------------------------------------------------

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "IOS Lab", nullptr, nullptr);
	if (window == nullptr) { std::cerr << "Failed to create GLFW window" << std::endl; glfwTerminate(); return EXIT_FAILURE; }
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) { std::cerr << "Failed to initialize GLEW" << std::endl; return EXIT_FAILURE; }

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);


	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

    lightingShader.Use();
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.specular"), 1);


	// --- Carga de Modelos ---
    size_t currentIndexAnimatable = 0;

    // 1. Laboratorio (NO se anima, NO se añade a animatableModels)
    Laboratorio_permanente = new Model((char*)"Models/laboratorio.obj");

    // 2. Modelos que SÍ se animan y desaparecen
    Model* pAire = new Model((char*)"Models/aire.obj");
    animatableModels.push_back(pAire);
    glm::mat4 transformAire = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 9.0f, 11.0f)) *
                             glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
    originalModelTransforms[pAire] = transformAire;
    modelToIndexMap[pAire] = currentIndexAnimatable++;

    Model* pLogoIOS = new Model((char*)"Models/logoIOS2.obj");
    animatableModels.push_back(pLogoIOS);
    glm::mat4 transformLogoIOS = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 5.5f, 11.0f)) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.5f, 1.0f));
    originalModelTransforms[pLogoIOS] = transformLogoIOS;
    modelToIndexMap[pLogoIOS] = currentIndexAnimatable++;

   
    // --- Para la PRIMERA PANTALLA ---
    Model* pPantalla1_visual = new Model((char*)"Models/pantalla.obj"); // Cargar el modelo
    animatableModels.push_back(pPantalla1_visual); // Añadir a la lista de animación
    glm::mat4 transformPantalla1 = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 4.0f, 6.0f)) *
                                  glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                  glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    originalModelTransforms[pPantalla1_visual] = transformPantalla1;
    modelToIndexMap[pPantalla1_visual] = currentIndexAnimatable++;

    // Carga el resto de tus modelos que quieres que desaparezcan:
    Model* pSilla = new Model((char*)"Models/silla.obj");
    animatableModels.push_back(pSilla);
    glm::mat4 transformSilla = glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, 0.17f, 2.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    originalModelTransforms[pSilla] = transformSilla; // Primera silla
    modelToIndexMap[pSilla] = currentIndexAnimatable++;

    Model* pProyector = new Model((char*)"Models/proye.obj");
    animatableModels.push_back(pProyector);
    glm::mat4 transformProyector = glm::translate(glm::mat4(1.0f), glm::vec3(-15.75f, 8.65f, 11.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.05f, 0.05f, 0.05f));
    originalModelTransforms[pProyector] = transformProyector;
    modelToIndexMap[pProyector] = currentIndexAnimatable++;

    Model* pEscritorio = new Model((char*)"Models/escritorio.obj");
    animatableModels.push_back(pEscritorio);
    glm::mat4 transformEscritorio = glm::translate(glm::mat4(1.0f), glm::vec3(-20.25f, 3.1f, 1.5f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    originalModelTransforms[pEscritorio] = transformEscritorio;
    modelToIndexMap[pEscritorio] = currentIndexAnimatable++;

    Model* pEscritorio2 = new Model((char*)"Models/escritorio2.obj");
    animatableModels.push_back(pEscritorio2);
    glm::mat4 transformEscritorio2 = glm::translate(glm::mat4(1.0f), glm::vec3(-22.0f, 0.25f, 15.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06));
    originalModelTransforms[pEscritorio2] = transformEscritorio2;
    modelToIndexMap[pEscritorio2] = currentIndexAnimatable++;

    Model* pEscritorio3 = new Model((char*)"Models/escritorio3.obj");
    animatableModels.push_back(pEscritorio3);
    glm::mat4 transformEscritorio3 = glm::translate(glm::mat4(1.0f), glm::vec3(-22.0f, 0.25f, 23.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(85.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(4.8f, 4.8f, 4.8f));
    originalModelTransforms[pEscritorio3] = transformEscritorio3;
    modelToIndexMap[pEscritorio3] = currentIndexAnimatable++;

    Model* pMesapequena = new Model((char*)"Models/mesapequena.obj");
    animatableModels.push_back(pMesapequena);
    glm::mat4 transformMesapequena = glm::translate(glm::mat4(1.0f), glm::vec3(5.75f, 0.18f, 22.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.7f, 2.5f));
    originalModelTransforms[pMesapequena] = transformMesapequena;
    modelToIndexMap[pMesapequena] = currentIndexAnimatable++;

    Model* pSillonNaranja = new Model((char*)"Models/sillonNaranja.obj");
    animatableModels.push_back(pSillonNaranja);
    glm::mat4 transformSillonNaranja = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.18f, 22.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    originalModelTransforms[pSillonNaranja] = transformSillonNaranja;
    modelToIndexMap[pSillonNaranja] = currentIndexAnimatable++;

    Model* pSillonGris = new Model((char*)"Models/sillonGris.obj");
    animatableModels.push_back(pSillonGris);
    glm::mat4 transformSillonGris = glm::translate(glm::mat4(1.0f), glm::vec3(7.5f, 0.18f, 22.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    originalModelTransforms[pSillonGris] = transformSillonGris;
    modelToIndexMap[pSillonGris] = currentIndexAnimatable++;

    Model* pPizarron = new Model((char*)"Models/pizarra.obj");
    animatableModels.push_back(pPizarron);
    glm::mat4 transformPizarron = glm::translate(glm::mat4(1.0f), glm::vec3(-18.75f, 2.46f, 23.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.2f, 1.8f, 2.0f));
    originalModelTransforms[pPizarron] = transformPizarron;
    modelToIndexMap[pPizarron] = currentIndexAnimatable++;

    Model* pLogoUNAM = new Model((char*)"Models/logoUNAM.obj");
    animatableModels.push_back(pLogoUNAM);
    glm::mat4 transformLogoUNAM = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 6.2f, 23.8f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    originalModelTransforms[pLogoUNAM] = transformLogoUNAM;
    modelToIndexMap[pLogoUNAM] = currentIndexAnimatable++;

    // Inicializar escalas para los modelos animables
    modelScales.resize(animatableModels.size(), 1.0f);


	// VAO y VBO para las lámparas
	GLuint lampVAO, lampVBO;
	glGenVertexArrays(1, &lampVAO);
	glGenBuffers(1, &lampVBO);
	glBindVertexArray(lampVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0); // Desvincular lampVAO

	glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = static_cast<GLfloat>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Actualizar Animación de Desaparición ---
        if (startDisappearAnimation && !modelsVanished)
        {
            bool allSmall = true;
            for (size_t i = 0; i < modelScales.size(); ++i)
            {
                if (modelScales[i] > 0.0f)
                {
                    modelScales[i] -= disappearSpeed * deltaTime;
                    if (modelScales[i] < 0.0f) modelScales[i] = 0.0f;
                    allSmall = false; // Al menos un modelo aún no es cero
                }
            }
            if (allSmall)
            {
                modelsVanished = true;
                startDisappearAnimation = false; // Detener la animación de encogimiento
                std::cout << "Modelos desaparecieron. Listo para cargar nuevos modelos." << std::endl;
                
            }
        }
        // --------------------------------------------

        lightingShader.Use();
        glUniform3fv(glGetUniformLocation(lightingShader.Program, "viewPos"), 1, glm::value_ptr(camera.GetPosition()));
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // --- Configuración de Luces (DirLight, PointLights) ---
        // Luz Direccional
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.7f, 0.7f, 0.7f); // Ambiente más sutil
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.5f, 0.5f, 0.5f);
        // Luces Puntuales
        glm::vec3 lightColor;
        lightColor.x = abs(sin(glfwGetTime() * Light1_Color_Factors.x));
        lightColor.y = abs(sin(glfwGetTime() * Light1_Color_Factors.y));
        lightColor.z = abs(sin(glfwGetTime() * Light1_Color_Factors.z)); // Usar abs para evitar colores negativos

        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.045f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.075f);

        // Point light 2
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), 0.0f, 0.05f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.0f);

        // Point light 3
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.0f);

        // Point light 4
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.10f, 0.0f, 0.10f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 0.10f, 0.0f, 0.10f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.0f);

        // SpotLight (Linterna)
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);

        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(12.0f)));




        // --- Dibujar Modelos ---
        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model"); // Obtener una vez
        glm::mat4 modelMatrix;

        // 1. Dibujar Laboratorio (NO se anima)
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        if (Laboratorio_permanente) Laboratorio_permanente->Draw(lightingShader);
        glBindVertexArray(0); 

        // 2. Dibujar Modelos Animables (los que están en animatableModels)
        for (Model* modelPtr : animatableModels)
        {
            size_t index = modelToIndexMap[modelPtr]; // Obtener el índice para esta instancia de modelo
            float currentAnimatedScale = modelScales[index];

            if (currentAnimatedScale <= 0.001f && modelsVanished) continue; // No dibujar si ya es muy pequeño

            // Empezar con la transformación original específica de esta instancia de modelo
            modelMatrix = originalModelTransforms[modelPtr];

            // Aplicar la escala de animación encima de la transformación original
            modelMatrix = glm::scale(modelMatrix, glm::vec3(currentAnimatedScale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

            if (currentAnimatedScale > 0.001f) {
                modelPtr->Draw(lightingShader);
            }
            glBindVertexArray(0); // Buena práctica
        }



        // Ejemplo para dibujar la SEGUNDA PANTALLA (usando el mismo pPantalla1_visual pero con otra transform):
        if (pPantalla1_visual) { // Asegurarse que pPantalla1_visual (o el nombre que le diste) existe
            size_t pantallaIndex = modelToIndexMap[pPantalla1_visual];
            float pantallaAnimScale = modelScales[pantallaIndex];

            if (pantallaAnimScale > 0.001f || (!startDisappearAnimation && !modelsVanished)) { // Dibujar si es visible o la animación no ha empezado
                // SEGUNDA TRANSFORMACIÓN PARA PANTALLA
                glm::mat4 transformPantalla2 = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 4.0f, 16.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));

                modelMatrix = glm::scale(transformPantalla2, glm::vec3(pantallaAnimScale)); // Aplicar escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pPantalla1_visual->Draw(lightingShader); // Dibujar el mismo objeto Model*
                glBindVertexArray(0);
            }
        }


        if (pSilla) { 
            size_t sillaAnimIndex = modelToIndexMap[pSilla];
            float sillaCurrentScale = modelScales[sillaAnimIndex];

            if (sillaCurrentScale > 0.001f || (!startDisappearAnimation && !modelsVanished)) {
                // Silla 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-20.2f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f)); // Escala original de esta instancia de silla
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Silla 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-14.5f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-11.75f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 5
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.5f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 6
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.75f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 7
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(3.0f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 8
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(5.75f, 0.17f, 2.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 9
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-22.9f, 0.17f, 15.0f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 10
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-20.5f, 0.17f, 15.0));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 11
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-10.75f, 0.17f, 13.75f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 12
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-7.75f, 0.17f, 13.75f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 13
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-4.75f, 0.17f, 13.75f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);
            }
        }

        if (pEscritorio) { 
            size_t escritorioAnimIndex = modelToIndexMap[pEscritorio];
            float escritorioCurrentScale = modelScales[escritorioAnimIndex];

            if (escritorioCurrentScale > 0.001f || (!startDisappearAnimation && !modelsVanished)) {
				// Escritorio 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-11.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Escritorio 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(5.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);
            }
        }

        if (pEscritorio2) {
            size_t escritorio2AnimIndex = modelToIndexMap[pEscritorio2];
            float escritorio2CurrentScale = modelScales[escritorio2AnimIndex];

            if (escritorio2CurrentScale > 0.001f || (!startDisappearAnimation && !modelsVanished)) {
                // Escritorio 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-9.57f, 0.25f, 8.75f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Escritorio 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.0f, 0.25f, 8.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.57f, 0.25f, 15.75f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(3.83f, 0.25f, 15.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(220.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);
            }
        }

        if (pSillonNaranja) {
            size_t sillonNaranjaAnimIndex = modelToIndexMap[pSillonNaranja];
            float sillonNarnajaCurrentScale = modelScales[sillonNaranjaAnimIndex];

            if (sillonNarnajaCurrentScale > 0.001f || (!startDisappearAnimation && !modelsVanished)) {
             
                // SillonNaranja 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNarnajaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // SillonNaranja 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNarnajaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // SillonNaranja 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-6.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNarnajaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);
            }
        }


        // --- Dibujar Lámparas (como en tu archivo, para 4 luces) ---
        lampShader.Use();
        GLint lampModelLoc = glGetUniformLocation(lampShader.Program, "model"); // Ubicación diferente para el shader de la lámpara
        GLint lampViewLoc = glGetUniformLocation(lampShader.Program, "view");
        GLint lampProjLoc = glGetUniformLocation(lampShader.Program, "projection");
        glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lampVAO);
        glm::mat4 lampModelMatrix; // Renombrada para evitar confusión
        for (GLuint i = 0; i < 4; i++) // Iterar sobre tus 4 luces
        {
            lampModelMatrix = glm::mat4(1.0f);
            lampModelMatrix = glm::translate(lampModelMatrix, pointLightPositions[i]);
            lampModelMatrix = glm::scale(lampModelMatrix, glm::vec3(0.2f)); // Escala original de la lámpara
            glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(lampModelMatrix));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

  
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (action == GLFW_PRESS) keys[key] = true;
    else if (action == GLFW_RELEASE) keys[key] = false;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS) // Tecla 'P' para la animación
    {
        if (!startDisappearAnimation && !modelsVanished) {
            startDisappearAnimation = true;
            std::cout << "Iniciando animación de desaparición..." << std::endl;
        }
        else if (modelsVanished) {
            std::cout << "Modelos desaparecieron. Presiona P para resetear (ejemplo)." << std::endl;
            // Ejemplo de reseteo:
            modelsVanished = false;
            startDisappearAnimation = false; // Para permitir que la animación se reinicie
            for (size_t i = 0; i < modelScales.size(); ++i) {
                modelScales[i] = 1.0f; // Restaurar escalas
            }
            std::cout << "Escena reseteada." << std::endl;
        }
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) // Control de luz parpadeante
    {
        active = !active; 
        if (active)
            Light1_Color_Factors = glm::vec3(1.0f, 0.5f, 0.2f);
        else
            Light1_Color_Factors = glm::vec3(0.0f); 
    }
}


void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    
    if (firstMouse) { lastX = static_cast<GLfloat>(xPos); lastY = static_cast<GLfloat>(yPos); firstMouse = false; }
    GLfloat xOffset = static_cast<GLfloat>(xPos) - lastX;
    GLfloat yOffset = lastY - static_cast<GLfloat>(yPos);
    lastX = static_cast<GLfloat>(xPos);
    lastY = static_cast<GLfloat>(yPos);
    camera.ProcessMouseMovement(xOffset, yOffset);
}


void DoMovement()
{
 
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);

    if (keys[GLFW_KEY_T]) pointLightPositions[0].x += 5.0f * deltaTime;
    if (keys[GLFW_KEY_G]) pointLightPositions[0].x -= 5.0f * deltaTime;
    if (keys[GLFW_KEY_Y]) pointLightPositions[0].y += 5.0f * deltaTime;
    if (keys[GLFW_KEY_H]) pointLightPositions[0].y -= 5.0f * deltaTime;
    if (keys[GLFW_KEY_U]) pointLightPositions[0].z -= 5.0f * deltaTime;
    if (keys[GLFW_KEY_J]) pointLightPositions[0].z += 5.0f * deltaTime;
}