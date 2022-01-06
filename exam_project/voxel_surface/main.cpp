#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

#include <vector>
#include <chrono>

#include "shader.h"
#include "camera.h"
#include "glmutils.h"
#include "opengl_debug.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "PerlinLikeNoise.h"
#include "primitives.h"


// structure to hold render info
// -----------------------------
struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
    void drawSceneObject() const{
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};


// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions,
                               const std::vector<float> &instancingOffsets = std::vector<float>(),
                               const std::vector<unsigned int> &indices = std::vector<unsigned int>(),
                               const std::vector<float> &normals = std::vector<float>(),
                               const std::vector<float> &colors = std::vector<float>());
void setup();
void drawObjects();
void drawSkybox();
void createVoxelLandscape();
void printControls();
unsigned int createSkybox();
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadTexture(char const * path);

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void key_input_callback(GLFWwindow* window, int key, int scanCode, int action, int mods);

// screen settings
// ---------------
unsigned int screenWidth = 1080;
unsigned int screenHeight = 1080;

float currentTime = 0;

float lastX = (float)screenWidth / 2.0;
float lastY = (float)screenHeight / 2.0;
Camera camera(glm::vec3(0.f, 32.f, 0.f));
glm::vec3 sunLightDiffuseColor = glm::vec3(0.9f, 0.6f, 0.5f);
glm::vec3 sunLightSpecular = glm::vec3(0.9f, 0.9f, 0.9f);
glm::vec3 sunLightAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
float sunLightIntensity = 0.8;
float sunRotation = 0.f;
float sunRotationSpeed = 36.f;

struct InstancedSceneObject{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int vertexCount;
    unsigned int instanceCount;

    void drawSceneObject(Shader *shader, glm::vec3 chunkOffset) const{
        glm::vec3 front;
        front.x = cos( glm::radians(0.f)) * cos(glm::radians(sunRotation));
        front.y = sin(glm::radians(sunRotation));
        front.z = sin(glm::radians(0.f)) * cos(glm::radians(sunRotation));
        glm::vec3 normalizedFront = glm::normalize(front);

        shader->use();
        shader->setMat4("viewProjectionMatrix", camera.getViewProjectionMatrix(screenWidth, screenHeight));
        shader->setMat4("viewMatrix", camera.GetViewMatrix());
        shader->setVec3("sunLightDiffuseColor", sunLightDiffuseColor);
        shader->setVec3("sunLightSpecular", sunLightSpecular);
        shader->setVec3("sunLightAmbient", sunLightAmbient);
        shader->setVec3("sunLightDirection", normalizedFront);
        shader->setFloat("sunLightIntensity", sunLightIntensity);
        shader->setVec3("chunkOffset", chunkOffset);
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
    }
};

// global variables used for rendering
// -----------------------------------
PerlinLikeNoise noise;
Shader* shaderProgram;
Shader* shaderProgramSkybox;

InstancedSceneObject instancedCube;
unsigned int skyboxVAO;
unsigned int cubemapTexture;
bool enableSkybox = false;
bool enableDayNightCycle = false;

int perlinWidth = 256;
int perlinHeight = 256;
int octaveCount = 5;
float bias = 1.f;
float heightScalar = 32.f;
float loopInterval = 0.f;
float deltaTime = 0.f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_input_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // setup mesh objects
    // ---------------------------------------
    setup();

    GLCall(glDepthRange(-1,1)); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    // render loop
    // -----------
    // render every loopInterval seconds
    loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();
    printControls();

    while (!glfwWindowShouldClose(window))
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (enableDayNightCycle) sunRotation += sunRotationSpeed * deltaTime;
        createVoxelLandscape();
        if (enableSkybox) drawSkybox();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;

        std::stringstream str;
        str << 1/elapsed.count();
        glfwSetWindowTitle(window, str.str().c_str());

        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
            deltaTime = elapsed.count();
        }
    }

    delete shaderProgram;
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void createVoxelLandscape()
{
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(0));
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(perlinWidth, 0, 0));
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(-perlinWidth, 0, 0));

    instancedCube.drawSceneObject(shaderProgram, glm::vec3(0, 0, perlinHeight));
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(0, 0, -perlinHeight));

    instancedCube.drawSceneObject(shaderProgram, glm::vec3(perlinWidth, 0, perlinHeight));
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(-perlinWidth, 0, -perlinHeight));

    instancedCube.drawSceneObject(shaderProgram, glm::vec3(-perlinWidth, 0, perlinHeight));
    instancedCube.drawSceneObject(shaderProgram, glm::vec3(perlinWidth, 0, -perlinHeight));
}

unsigned int createSkybox()
{
    cubemapTexture = loadCubemap(faces);

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    return skyboxVAO;
}

void drawSkybox()
{
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    shaderProgramSkybox->use();
    camera.getViewProjectionMatrix(screenWidth, screenHeight);
    shaderProgramSkybox->setMat4("projection", camera.projectionMatrix);
    shaderProgramSkybox->setMat4("view", camera.GetViewMatrix());
    shaderProgramSkybox->setInt("skybox", 0);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

std::vector<float> createInstancingOffsets()
{
    auto perlinNoise = noise.Noise2D(perlinWidth, perlinHeight, &noise.seedVector2D, octaveCount, bias);
    std::vector<float> instancingOffsets;

    for (int x = 0; x < perlinWidth; x++)
    {
        for (int z = 0; z < perlinHeight; z++)
        {
            float y = noise.noiseVector2D[z * perlinWidth + x] * 2 -1;

            instancingOffsets.push_back(x - perlinWidth/2);
            instancingOffsets.push_back(glm::round(y * heightScalar));
            instancingOffsets.push_back(z - perlinHeight/2);
        }
    }
    return instancingOffsets;
}

void setup(){
    // initialize shaders
    shaderProgram = new Shader("shaders/default.vert", "shaders/default.frag");
    shaderProgramSkybox = new Shader("shaders/skybox.vert", "shaders/skybox.frag");

    std::vector<float> offsets = createInstancingOffsets();
    instancedCube.VAO = createVertexArray(vertices, offsets);
    instancedCube.vertexCount = vertices.size()/6;
    instancedCube.instanceCount = perlinWidth * perlinHeight;

    skyboxVAO = createSkybox();
}

unsigned int createVertexArray(
        const std::vector<float> &positions,
        const std::vector<float> &instancingOffsets,
        const std::vector<unsigned int> &indices,
        const std::vector<float> &normals,
        const std::vector<float> &colors)
{
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(vertices); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(shaderProgram->ID, "pos");
    int normalAttributeLocation = glGetAttribLocation(shaderProgram->ID, "aNormal");

    glEnableVertexAttribArray(posAttributeLocation);
    glEnableVertexAttribArray(normalAttributeLocation);

    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);
    glVertexAttribPointer(normalAttributeLocation, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

    // set vertex shader attribute "instancingOffset"
    if (!instancingOffsets.empty())
    {
        instancedCube.VBO = createArrayBuffer(instancingOffsets);
        int offsetAttributeLocation = glGetAttribLocation(shaderProgram->ID, "instancingOffsets");
        glEnableVertexAttribArray(offsetAttributeLocation);
        glVertexAttribPointer(offsetAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(offsetAttributeLocation, 1);
    }

    return VAO;
}

unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(float), array.data(), GL_STATIC_DRAW);

    return VBO;
}

void updateVBO(const std::vector<float> &array, unsigned int ID)
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(float), array.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), array.data(), GL_STATIC_DRAW);

    return EBO;
}

// NEW!
// instead of using the NDC to transform from screen space you can now define the range using the
// min and max parameters
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y){
    float sum = max - min;
    float xInRange = (float) screenX / (float) screenW * sum - sum/2.0f;
    float yInRange = (float) screenY / (float) screenH * sum - sum/2.0f;
    x = xInRange;
    y = -yInRange; // flip screen space y axis
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){

    // camera rotation
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = posX;
        lastY = posY;
        firstMouse = false;
    }

    float xoffset = posX - lastX;
    float yoffset = lastY - posY; // reversed since y-coordinates go from bottom to top

    lastX = posX;
    lastY = posY;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void printControls()
{
    std::cout << "Control keys:" << std::endl;
    std::cout << "1: OctaveCount: " << octaveCount << std::endl;
    std::cout << "2: Bias: " << bias << std::endl;
    std::cout << "3: HeightScalar: " << heightScalar << std::endl;
    std::cout << "4: Reseed" << std::endl;
    std::cout << "5: Toggle Day/Night cycle" << std::endl;
    std::cout << "6: Toggle Day/Night cycle" << std::endl;
    std::cout << std::endl;
}

void key_input_callback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    switch (key)
    {
        case GLFW_KEY_1:
            if (action == GLFW_RELEASE){
                if (octaveCount >= 8.f)
                {
                    octaveCount = 1.f;
                } else octaveCount++;

                std::cout<< "Pressed 1: OctaveCount: " << octaveCount << std::endl;
                auto offsets = createInstancingOffsets();
                updateVBO(offsets, instancedCube.VBO);
            }
            break;
        case GLFW_KEY_2:
            if (action == GLFW_RELEASE){
                if (bias >= 3.f)
                {
                    bias = .5f;
                } else bias += 0.25f;

                std::cout<< "Pressed 2: Bias: " << bias << std::endl;
                auto offsets = createInstancingOffsets();
                updateVBO(offsets, instancedCube.VBO);
            }
            break;
        case GLFW_KEY_3:
            if (action == GLFW_RELEASE){
                if (heightScalar >= 256.f)
                {
                    heightScalar = 2.f;
                } else heightScalar *= 2;

                std::cout<< "Pressed 3: HeightScalar: " << heightScalar << std::endl;
                auto offsets = createInstancingOffsets();
                updateVBO(offsets, instancedCube.VBO);
            }
            break;
        case GLFW_KEY_4:
            if (action == GLFW_RELEASE){
                std::cout<< "Pressed 4: Reseed" << std::endl;
                noise.reseed();
                auto offsets = createInstancingOffsets();
                updateVBO(offsets, instancedCube.VBO);
            }
            break;
        case GLFW_KEY_5:
            if (action == GLFW_RELEASE){
                std::cout<< "Pressed 5: Toggle Skybox" << std::endl;
                enableSkybox = !enableSkybox;
            }
            break;
        case GLFW_KEY_6:
            if (action == GLFW_RELEASE){
                std::cout<< "Pressed 6: Toggle Day/Night cycle" << std::endl;
                enableDayNightCycle = !enableDayNightCycle;
            }
            break;
        default:
            break;
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // movement commands
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    camera.getViewProjectionMatrix(width, height); // this sets the projectionMatrix inside the camera class as a side effect
    glViewport(0, 0, width, height);
}

// Load the skybox textures from the given cubeFaces
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
