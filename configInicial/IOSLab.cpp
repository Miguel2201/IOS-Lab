#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <map>


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
Camera  camera(glm::vec3(28.0f, 6.0f, 9.5f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
bool active; // Para la luz parpadeante

// Point Lights
glm::vec3 pointLightPositions[] = {
    glm::vec3(-22.75f, 9.25f, 11.0f), glm::vec3(-18.75f, 9.25f, 11.0f),
    glm::vec3(-14.75f, 9.25f, 11.0f), glm::vec3(-8.75f, 9.25f, 11.0f)
};
float lampVertices[] = { 
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
};
glm::vec3 Light1_Color_Factors = glm::vec3(0);

// Deltatime
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// --- Variables para Animación de Desaparición (Escena Antigua) ---
bool startDisappearAnimation = false;
bool oldModelsVanished = false; // Renombrado para claridad
const float disappearSpeed = 0.7f;
std::vector<Model*> animatableOldModels; // Modelos que desaparecen
std::vector<float> oldModelScales;
std::map<Model*, glm::mat4> originalOldModelTransforms;
std::map<Model*, size_t> oldModelToIndexMap;
Model* Laboratorio_permanente = nullptr;

// ---------------------------------------------------------------
// --- Variables para Animación de Aparición (Escena Nueva) ---
enum class AppearPhase { IDLE, GROWING, MOVING, FINISHED };
bool startAppearAnimationGlobal = false;
int appearingModelIndex = -1;

// Definir un punto de aparición FIJO, fuera del laboratorio, cerca de la pos inicial de la cámara
const glm::vec3 FIXED_SPAWN_POINT = glm::vec3(25.0f, 2.5f, 9.5f); // AJUSTA ESTAS COORDENADAS


struct NewModelAnimated {
    Model* modelPtr = nullptr;
    glm::mat4 targetTransform = glm::mat4(1.0f);
    glm::vec3 finalModelScale = glm::vec3(1.0f);

    glm::vec3 spawnPosition = FIXED_SPAWN_POINT; // Usará el punto fijo
    glm::quat spawnOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 currentPosition = glm::vec3(0.0f);
    float currentScaleFactor = 0.0f;
    float movementProgress = 0.0f;
    AppearPhase phase = AppearPhase::IDLE;
    bool isLoaded = false;
    std::string path;

    NewModelAnimated(const std::string& p, const glm::mat4& target, const glm::vec3& finalScaleVec)
        : path(p), targetTransform(target), finalModelScale(finalScaleVec) {
        // spawnPosition se establece a FIXED_SPAWN_POINT por defecto
    }

    void Load() {
        if (!isLoaded && !path.empty()) {
            modelPtr = new Model((char*)path.c_str());
            isLoaded = true;
            std::cout << "Cargado nuevo modelo: " << path << std::endl;
        }
    }

    // Inicia la animación para este modelo
    void StartAnimation() { 
        if (!isLoaded) Load();
        if (!modelPtr) return;

        // La orientación inicial puede ser la orientación final del modelo
        spawnOrientation = glm::normalize(glm::quat_cast(targetTransform));

        currentPosition = spawnPosition; // Ya está establecido a FIXED_SPAWN_POINT
        currentScaleFactor = 0.0f;
        movementProgress = 0.0f;
        phase = AppearPhase::GROWING;
        std::cout << "Iniciando aparición de: " << path << " en ("
            << spawnPosition.x << ", " << spawnPosition.y << ", " << spawnPosition.z << ")" << std::endl;
    }
};
std::vector<NewModelAnimated> newSceneModels;
const float growSpeed = 0.3;    // Un poco más lento para ver mejor
const float newModelMoveDuration = 1.0f; // Un poco más de tiempo para moverse
// -------------------------------------------------------------

Model* PantallaNueva_Test = nullptr; 

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

    // --- Carga de Modelos INICIALES (que desaparecerán) ---
    size_t currentIndexOldAnimatable = 0;
    Laboratorio_permanente = new Model((char*)"Models/laboratorio.obj");

    Model* pAire = new Model((char*)"Models/aire.obj");
    animatableOldModels.push_back(pAire);
    originalOldModelTransforms[pAire] = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 9.0f, 11.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
    oldModelToIndexMap[pAire] = currentIndexOldAnimatable++;
    
    Model* pLogoIOS = new Model((char*)"Models/logoIOS2.obj");
    animatableOldModels.push_back(pLogoIOS);
    originalOldModelTransforms[pLogoIOS] = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 5.5f, 11.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.5f, 1.0f));
    oldModelToIndexMap[pLogoIOS] = currentIndexOldAnimatable++;

    Model* pPantalla1_visual = new Model((char*)"Models/pantalla.obj");
    animatableOldModels.push_back(pPantalla1_visual);
    originalOldModelTransforms[pPantalla1_visual] = glm::translate(glm::mat4(1.0f), glm::vec3(-25.75f, 4.0f, 6.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    oldModelToIndexMap[pPantalla1_visual] = currentIndexOldAnimatable++;

    Model* pSilla = new Model((char*)"Models/silla.obj");
    animatableOldModels.push_back(pSilla);
    originalOldModelTransforms[pSilla] = glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, 0.17f, 2.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    oldModelToIndexMap[pSilla] = currentIndexOldAnimatable++;

    Model* pProyector = new Model((char*)"Models/proye.obj");
    animatableOldModels.push_back(pProyector);
    glm::mat4 transformProyector = glm::translate(glm::mat4(1.0f), glm::vec3(-15.75f, 8.65f, 11.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.05f, 0.05f, 0.05f));
    originalOldModelTransforms[pProyector] = transformProyector;
    oldModelToIndexMap[pProyector] = currentIndexOldAnimatable++;

    Model* pEscritorio = new Model((char*)"Models/escritorio.obj");
    animatableOldModels.push_back(pEscritorio);
    glm::mat4 transformEscritorio = glm::translate(glm::mat4(1.0f), glm::vec3(-20.25f, 3.1f, 1.5f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    originalOldModelTransforms[pEscritorio] = transformEscritorio;
    oldModelToIndexMap[pEscritorio] = currentIndexOldAnimatable++;

    Model* pEscritorio2 = new Model((char*)"Models/escritorio2.obj");
    animatableOldModels.push_back(pEscritorio2);
    glm::mat4 transformEscritorio2 = glm::translate(glm::mat4(1.0f), glm::vec3(-22.0f, 0.25f, 15.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06));
    originalOldModelTransforms[pEscritorio2] = transformEscritorio2;
    oldModelToIndexMap[pEscritorio2] = currentIndexOldAnimatable++;

    Model* pEscritorio3 = new Model((char*)"Models/escritorio3.obj");
    animatableOldModels.push_back(pEscritorio3);
    glm::mat4 transformEscritorio3 = glm::translate(glm::mat4(1.0f), glm::vec3(-22.0f, 0.25f, 23.2f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(85.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(4.8f, 4.8f, 4.8f));
    originalOldModelTransforms[pEscritorio3] = transformEscritorio3;
    oldModelToIndexMap[pEscritorio3] = currentIndexOldAnimatable++;

    Model* pMesapequena = new Model((char*)"Models/mesapequena.obj");
    animatableOldModels.push_back(pMesapequena);
    glm::mat4 transformMesapequena = glm::translate(glm::mat4(1.0f), glm::vec3(5.75f, 0.18f, 22.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.7f, 2.5f));
    originalOldModelTransforms[pMesapequena] = transformMesapequena;
    oldModelToIndexMap[pMesapequena] = currentIndexOldAnimatable++;

    Model* pSillonNaranja = new Model((char*)"Models/sillonNaranja.obj");
    animatableOldModels.push_back(pSillonNaranja);
    glm::mat4 transformSillonNaranja = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.18f, 22.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    originalOldModelTransforms[pSillonNaranja] = transformSillonNaranja;
    oldModelToIndexMap[pSillonNaranja] = currentIndexOldAnimatable++;

    Model* pSillonGris = new Model((char*)"Models/sillonGris.obj");
    animatableOldModels.push_back(pSillonGris);
    glm::mat4 transformSillonGris = glm::translate(glm::mat4(1.0f), glm::vec3(7.5f, 0.18f, 22.5f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    originalOldModelTransforms[pSillonGris] = transformSillonGris;
    oldModelToIndexMap[pSillonGris] = currentIndexOldAnimatable++;

    Model* pPizarron = new Model((char*)"Models/pizarra.obj");
    animatableOldModels.push_back(pPizarron);
    glm::mat4 transformPizarron = glm::translate(glm::mat4(1.0f), glm::vec3(-18.75f, 2.46f, 23.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(2.2f, 1.8f, 2.0f));
    originalOldModelTransforms[pPizarron] = transformPizarron;
    oldModelToIndexMap[pPizarron] = currentIndexOldAnimatable++;

    Model* pLogoUNAM = new Model((char*)"Models/logoUNAM.obj");
    animatableOldModels.push_back(pLogoUNAM);
    glm::mat4 transformLogoUNAM = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 6.2f, 23.8f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    originalOldModelTransforms[pLogoUNAM] = transformLogoUNAM;
    oldModelToIndexMap[pLogoUNAM] = currentIndexOldAnimatable++;


    oldModelScales.resize(animatableOldModels.size(), 1.0f);

    // --- Definir los NUEVOS modelos que aparecerán ---
    glm::mat4 pantallaNuevaTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-24.75f, 0.2f, 16.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
    glm::vec3 pantallaNuevaFinalScale = glm::vec3(2.5f, 2.5f, 2.5f);
    newSceneModels.emplace_back("Models/pantallaNueva.obj", pantallaNuevaTargetTransform, pantallaNuevaFinalScale);

    glm::mat4 pantallaNueva2TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-24.75f, 0.2f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
    glm::vec3 pantallaNueva2FinalScale = glm::vec3(2.5f, 2.5f, 2.5f);
    newSceneModels.emplace_back("Models/pantallaNueva.obj", pantallaNueva2TargetTransform, pantallaNueva2FinalScale);

    glm::mat4 pantallaproyectorTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-12.75f, 0.2f, 23.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
    glm::vec3 pantallaproyectorFinalScale = glm::vec3(2.5f, 2.5f, 2.5f);
    newSceneModels.emplace_back("Models/pantallaproyector.obj", pantallaproyectorTargetTransform, pantallaproyectorFinalScale);

    glm::mat4 logoIOSTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-25.74f, 5.5f, 11.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.1f, 1.3f, 1.1f));
    glm::vec3 logoIOSFinalScale = glm::vec3(1.1f, 1.3f, 1.1f);
    newSceneModels.emplace_back("Models/logoIOS2.obj", logoIOSTargetTransform, logoIOSFinalScale);

    glm::mat4 logoUNAMTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 6.2f, 23.8f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    glm::vec3 logoUNAMFinalScale = glm::vec3(1.3f, 1.3f, 1.3f);
    newSceneModels.emplace_back("Models/logoUNAM.obj", logoUNAMTargetTransform, logoUNAMFinalScale);

    glm::mat4 escritorioTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-20.25f, 3.35f, 1.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    glm::vec3 escritorioFinalScale = glm::vec3(1.25f, 1.1f, 1.2f);
    newSceneModels.emplace_back("Models/escritorio.obj", escritorioTargetTransform, escritorioFinalScale);

    glm::mat4 escritorio_2TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-11.75f, 3.35f, 1.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    glm::vec3 escritorio_2FinalScale = glm::vec3(1.25f, 1.1f, 1.2f);
    newSceneModels.emplace_back("Models/escritorio.obj", escritorio_2TargetTransform, escritorio_2FinalScale);

    glm::mat4 escritorio_3TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 3.35f, 1.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    glm::vec3 escritorio_3FinalScale = glm::vec3(1.25f, 1.1f, 1.2f);
    newSceneModels.emplace_back("Models/escritorio.obj", escritorio_3TargetTransform, escritorio_3FinalScale);

    glm::mat4 escritorio_4TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(5.75f, 3.35f, 1.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
    glm::vec3 escritorio_4FinalScale = glm::vec3(1.25f, 1.1f, 1.2f);
    newSceneModels.emplace_back("Models/escritorio.obj", escritorio_4TargetTransform, escritorio_4FinalScale);

    glm::mat4 sillaTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 sillaFinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", sillaTargetTransform, sillaFinalScale);

    glm::mat4 silla2TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-20.2f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla2FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla2TargetTransform, silla2FinalScale);

    glm::mat4 silla3TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-14.5f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla3FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla3TargetTransform, silla3FinalScale);

    glm::mat4 silla4TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-11.75f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla4FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla4TargetTransform, silla4FinalScale);

    glm::mat4 silla5TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-5.5f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla5FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla5TargetTransform, silla5FinalScale);

    glm::mat4 silla6TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.75f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla6FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla6TargetTransform, silla6FinalScale);

    glm::mat4 silla7TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla7FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla7TargetTransform, silla7FinalScale);

    glm::mat4 silla8TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(5.75f, 0.17f, 2.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla8FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla8TargetTransform, silla8FinalScale);
    
    glm::mat4 silla9TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-22.9f, 0.17f, 15.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla9FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla9TargetTransform, silla9FinalScale);

    glm::mat4 silla10TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-20.25f, 0.17f, 15.2f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.092f, 0.092f, 0.092f));
    glm::vec3 silla10FinalScale = glm::vec3(0.092f, 0.092f, 0.092f);
    newSceneModels.emplace_back("Models/silla.obj", silla10TargetTransform, silla10FinalScale);

    glm::mat4 sillonNaranjaTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.18f, 22.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    glm::vec3 sillonNaranjaFinalScale = glm::vec3(2.6f, 2.8f, 2.6f);
    newSceneModels.emplace_back("Models/sillonNaranja.obj", sillonNaranjaTargetTransform, sillonNaranjaFinalScale);

    glm::mat4 sillonNaranja2TargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.18f, 22.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    glm::vec3 sillonNaranja2FinalScale = glm::vec3(2.6f, 2.8f, 2.6f);
    newSceneModels.emplace_back("Models/sillonNaranja.obj", sillonNaranja2TargetTransform, sillonNaranja2FinalScale);

    glm::mat4 sillonGrisTargetTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, 0.18f, 22.5f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
    glm::vec3 sillonGrisFinalScale = glm::vec3(2.6f, 2.8f, 2.6f);
    newSceneModels.emplace_back("Models/sillonGris.obj", sillonGrisTargetTransform, sillonGrisFinalScale);





    

    GLuint lampVAO, lampVBO;
    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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

        // --- Animación de DESAPARICIÓN de Modelos Antiguos ---
        if (startDisappearAnimation && !oldModelsVanished) {
            bool allOldSmall = true;
            if (!animatableOldModels.empty()) {
                for (size_t i = 0; i < oldModelScales.size(); ++i) {
                    if (oldModelScales[i] > 0.0f) {
                        oldModelScales[i] -= disappearSpeed * deltaTime;
                        if (oldModelScales[i] < 0.0f) oldModelScales[i] = 0.0f;
                    }
                    if (oldModelScales[i] > 0.001f) allOldSmall = false;
                }
            }
            if (allOldSmall) {
                oldModelsVanished = true;
                std::cout << "Modelos ANTIGUOS desaparecieron. Presiona 'O' para que aparezcan los nuevos." << std::endl;
            }
        }
        // --- Animación de APARICIÓN de Modelos Nuevos ---
        if (startAppearAnimationGlobal && oldModelsVanished && appearingModelIndex < (int)newSceneModels.size()) {
            if (appearingModelIndex == -1) {
                appearingModelIndex = 0;
                while (appearingModelIndex < (int)newSceneModels.size() &&
                    newSceneModels[appearingModelIndex].phase == AppearPhase::FINISHED) {
                    appearingModelIndex++;
                }
                if (appearingModelIndex < (int)newSceneModels.size()) {
                    newSceneModels[appearingModelIndex].StartAnimation(); 
                }
                else {
                    startAppearAnimationGlobal = false;
                }
            }

            if (appearingModelIndex < (int)newSceneModels.size()) {
                NewModelAnimated& currentNewModel = newSceneModels[appearingModelIndex];

                // Limitar deltaTime para suavizar saltos
                float cappedDeltaTime = glm::min(deltaTime, 0.033f); // ej. no más de ~30 FPS para la lógica de animación

                if (currentNewModel.modelPtr && currentNewModel.phase == AppearPhase::GROWING) {
                    currentNewModel.currentScaleFactor += growSpeed * cappedDeltaTime;
                    if (currentNewModel.currentScaleFactor >= 1.0f) {
                        currentNewModel.currentScaleFactor = 1.0f;
                        currentNewModel.phase = AppearPhase::MOVING;
                        currentNewModel.movementProgress = 0.0f;
                        
                        std::cout << "Modelo " << currentNewModel.path << " crecio. Iniciando movimiento." << std::endl;
                    }
                }
                else if (currentNewModel.modelPtr && currentNewModel.phase == AppearPhase::MOVING) {
                    glm::vec3 targetPos = glm::vec3(currentNewModel.targetTransform[3]);
                    // El punto de inicio para la interpolación es el spawnPosition
                    glm::vec3 startMovePos = currentNewModel.spawnPosition;

                    if (newModelMoveDuration > 0.001f) {
                        currentNewModel.movementProgress += (1.0f / newModelMoveDuration) * cappedDeltaTime;
                    }
                    else {
                        currentNewModel.movementProgress = 1.0f;
                    }

                    if (currentNewModel.movementProgress >= 1.0f) {
                        currentNewModel.movementProgress = 1.0f;
                        currentNewModel.phase = AppearPhase::FINISHED;
                        currentNewModel.currentPosition = targetPos; // Asegurar posición final
                        std::cout << "Modelo " << currentNewModel.path << " se movio a su posicion." << std::endl;

                        appearingModelIndex++;
                        if (appearingModelIndex < (int)newSceneModels.size()) {
                            newSceneModels[appearingModelIndex].StartAnimation(); // Iniciar siguiente
                        }
                        else {
                            startAppearAnimationGlobal = false;
                            std::cout << "Todos los nuevos modelos aparecieron." << std::endl;
                        }
                    }
                    else {
                        currentNewModel.currentPosition = glm::mix(startMovePos, targetPos, currentNewModel.movementProgress);
                    }
                }
            }
        }

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
        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        glm::mat4 modelMatrix;

        // --- Dibujar Laboratorio (permanente) ---
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        if (Laboratorio_permanente) Laboratorio_permanente->Draw(lightingShader);
        glBindVertexArray(0);

        // --- Dibujar Modelos ANTIGUOS (animables que desaparecen) ---
        if (!oldModelsVanished || startDisappearAnimation) { // Solo dibujar si no han desaparecido o la animación está activa
            for (Model* modelPtr : animatableOldModels) {
                size_t index = oldModelToIndexMap[modelPtr];
                float currentOldScale = oldModelScales[index];
                if (currentOldScale <= 0.001f && oldModelsVanished) continue;

                modelMatrix = originalOldModelTransforms[modelPtr];
                modelMatrix = glm::scale(modelMatrix, glm::vec3(currentOldScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                if (currentOldScale > 0.001f) modelPtr->Draw(lightingShader);
                glBindVertexArray(0);
            }
        }
        // --- Dibujar Modelos NUEVOS (animables que aparecen) ---
        if (oldModelsVanished) { // Empezar a dibujar/animar los nuevos solo cuando los viejos se fueron
            for (size_t i = 0; i < newSceneModels.size(); ++i) {
                NewModelAnimated& newModel = newSceneModels[i];
                if (newModel.modelPtr && newModel.phase != AppearPhase::IDLE) {
                    modelMatrix = glm::mat4(1.0f);

                    // Obtener rotación y posición finales de targetTransform
                    glm::quat finalRotation = glm::normalize(glm::quat_cast(newModel.targetTransform));
                    glm::vec3 finalPosition = glm::vec3(newModel.targetTransform[3]);

                    if (newModel.phase == AppearPhase::GROWING) {
                        modelMatrix = glm::translate(glm::mat4(1.0f), newModel.currentPosition); // En spawnPosition (cerca de la cámara)
                        modelMatrix *= glm::mat4_cast(newModel.spawnOrientation); // Orientación durante el crecimiento
                        modelMatrix = glm::scale(modelMatrix, newModel.finalModelScale * newModel.currentScaleFactor);
                    }
                    else if (newModel.phase == AppearPhase::MOVING) {
                        modelMatrix = glm::translate(glm::mat4(1.0f), newModel.currentPosition); // Posición interpolada
                        modelMatrix *= glm::mat4_cast(finalRotation); // Orientación final
                        modelMatrix = glm::scale(modelMatrix, newModel.finalModelScale);
                    }
                    else if (newModel.phase == AppearPhase::FINISHED) {
                        modelMatrix = glm::translate(glm::mat4(1.0f), finalPosition);
                        modelMatrix *= glm::mat4_cast(finalRotation);
                        modelMatrix = glm::scale(modelMatrix, newModel.finalModelScale);
                        
                    }

                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                    newModel.modelPtr->Draw(lightingShader);
                    glBindVertexArray(0);
                }
            }
        }


        if (pPantalla1_visual) { 
            size_t pantallaIndex = oldModelToIndexMap[pPantalla1_visual];
            float pantallaAnimScale = oldModelScales[pantallaIndex];

            if (pantallaAnimScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) { // Dibujar si es visible o la animación no ha empezado
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
            size_t sillaAnimIndex = oldModelToIndexMap[pSilla];
            float sillaCurrentScale = oldModelScales[sillaAnimIndex];

            if (sillaCurrentScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) {
                // Silla 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-20.2f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f)); // Escala original de esta instancia de silla
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader); // Dibuja el mismo objeto Model* Silla
                glBindVertexArray(0);

                // Silla 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-14.5f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-11.75f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 5
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.5f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 6
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.75f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 7
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(3.0f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 8
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(5.75f, 0.17f, 2.0f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 9 -- Escritorio2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-22.9f, 0.17f, 15.0f))*
                    glm::rotate(glm::mat4(1.0f), glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 10
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-20.25f, 0.17f, 15.2f))*
                    glm::rotate(glm::mat4(1.0f), glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 11
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-10.75f, 0.17f, 7.85f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 12
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-8.25f, 0.17f, 7.85f)) *
                    glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 13
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.75f, 0.17f, 13.75f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);

                // Silla 14
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(1.75f, 0.17f, 13.75f))*
                glm::scale(modelMatrix, glm::vec3(0.092f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillaCurrentScale));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSilla->Draw(lightingShader);
                glBindVertexArray(0);
            }
        }

        if (pEscritorio) { 
            size_t escritorioAnimIndex = oldModelToIndexMap[pEscritorio];
            float escritorioCurrentScale = oldModelScales[escritorioAnimIndex];

            if (escritorioCurrentScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) {
				// Escritorio 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-11.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); 
                glBindVertexArray(0);

                // Escritorio 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); 
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(5.75f, 3.1f, 1.5f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(1.25f, 1.1f, 1.2f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorioCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio->Draw(lightingShader); 
                glBindVertexArray(0);
            }
        }

        if (pEscritorio2) {
            size_t escritorio2AnimIndex = oldModelToIndexMap[pEscritorio2];
            float escritorio2CurrentScale = oldModelScales[escritorio2AnimIndex];

            if (escritorio2CurrentScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) {
                // Escritorio 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-9.57f, 0.25f, 8.74f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); 
                glBindVertexArray(0);

                // Escritorio 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-5.0f, 0.25f, 8.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(220.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); 
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.57f, 0.25f, 15.75f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); 
                glBindVertexArray(0);

                // Escritorio 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(3.83f, 0.25f, 15.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(220.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.10f, 0.06f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(escritorio2CurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pEscritorio2->Draw(lightingShader); 
                glBindVertexArray(0);
            }
        }

        if (pSillonNaranja) {
            size_t sillonNaranjaAnimIndex = oldModelToIndexMap[pSillonNaranja];
            float sillonNaranjaCurrentScale = oldModelScales[sillonNaranjaAnimIndex];

            if (sillonNaranjaCurrentScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) {
             
                // SillonNaranja 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNaranjaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); 
                glBindVertexArray(0);

                // SillonNaranja 3
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNaranjaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); 
                glBindVertexArray(0);

                // SillonNaranja 4
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-6.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonNaranjaCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonNaranja->Draw(lightingShader); 
                glBindVertexArray(0);
            }
        }

        if (pSillonGris) {
            size_t sillonGrisAnimIndex = oldModelToIndexMap[pSillonGris];
            float sillonGrisCurrentScale = oldModelScales[sillonGrisAnimIndex];

            if (sillonGrisCurrentScale > 0.001f || (!startDisappearAnimation && !oldModelsVanished)) {

                // SillonNaranja 2
                modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(4.0f, 0.18f, 22.5f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(2.6f, 2.8f, 2.6f));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sillonGrisCurrentScale)); // Escala animada
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                pSillonGris->Draw(lightingShader);
                glBindVertexArray(0);
            }

        }


        //lamparas
        lampShader.Use();
        GLint lampModelLoc = glGetUniformLocation(lampShader.Program, "model");
        GLint lampViewLoc = glGetUniformLocation(lampShader.Program, "view");
        GLint lampProjLoc = glGetUniformLocation(lampShader.Program, "projection");
        glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lampVAO);
        glm::mat4 lampModelMatrix;
        for (GLuint i = 0; i < 4; i++)
        {
            lampModelMatrix = glm::mat4(1.0f);
            lampModelMatrix = glm::translate(lampModelMatrix, pointLightPositions[i]);
            lampModelMatrix = glm::scale(lampModelMatrix, glm::vec3(0.2f));
            glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(lampModelMatrix));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
    // --- Limpieza ---
    delete Laboratorio_permanente;
    if (PantallaNueva_Test) delete PantallaNueva_Test; // Si lo usaste
    for (Model* m : animatableOldModels) { delete m; }
    animatableOldModels.clear();
    for (NewModelAnimated& nm : newSceneModels) { if (nm.modelPtr) delete nm.modelPtr; }
    newSceneModels.clear();
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glfwTerminate();
    return EXIT_SUCCESS;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    // ... (Lógica para teclas P y O sin cambios en la función StartAnimation) ...
    if (action == GLFW_PRESS) keys[key] = true;
    else if (action == GLFW_RELEASE) keys[key] = false;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_PRESS) { // Desaparición modelos viejos
        if (!startDisappearAnimation && !oldModelsVanished) {
            startDisappearAnimation = true;
            std::cout << "Iniciando animación de DESAPARICIÓN de modelos antiguos..." << std::endl;
        }
        else if (oldModelsVanished && !startAppearAnimationGlobal && (appearingModelIndex == -1 || (appearingModelIndex >= (int)newSceneModels.size() - 1 && (newSceneModels.empty() || newSceneModels.back().phase == AppearPhase::FINISHED)))) {
            oldModelsVanished = false;
            for (float& scale : oldModelScales) scale = 1.0f;
            std::cout << "Modelos antiguos reseteados." << std::endl;
        }
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) { // Tecla 'O' para la aparición
        if (oldModelsVanished && !startAppearAnimationGlobal) {
            bool allNewFinished = newSceneModels.empty() ? false : true; // Si no hay modelos, no está terminado
            for (const auto& nm : newSceneModels) {
                if (nm.phase != AppearPhase::FINISHED) {
                    allNewFinished = false;
                    break;
                }
            }

            if (appearingModelIndex == -1 || allNewFinished) {
                appearingModelIndex = -1;
                for (auto& nm : newSceneModels) {
                    nm.phase = AppearPhase::IDLE;
                    nm.currentScaleFactor = 0.0f;
                    nm.movementProgress = 0.0f;
                    nm.currentPosition = nm.spawnPosition; // Resetear posición al spawnPoint
                }
                std::cout << "Animación de aparición de nuevos modelos (re)iniciada." << std::endl;
            }
            startAppearAnimationGlobal = true;

        }
        else if (startAppearAnimationGlobal) {
            std::cout << "Animación de aparición ya en curso." << std::endl;
        }
        else if (!oldModelsVanished) {
            std::cout << "Los modelos antiguos aún no han desaparecido (Presiona P)." << std::endl;
        }
    }
    // ... (tu tecla SPACE para la luz parpadeante)
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        active = !active;
        if (active) Light1_Color_Factors = glm::vec3(1.0f, 0.5f, 0.2f);
        else Light1_Color_Factors = glm::vec3(1.0f);
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) { lastX = static_cast<GLfloat>(xPos); lastY = static_cast<GLfloat>(yPos); firstMouse = false; }
    GLfloat xOffset = static_cast<GLfloat>(xPos) - lastX;
    GLfloat yOffset = lastY - static_cast<GLfloat>(yPos);
    lastX = static_cast<GLfloat>(xPos);
    lastY = static_cast<GLfloat>(yPos);
    camera.ProcessMouseMovement(xOffset, yOffset);
}
void DoMovement() {
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