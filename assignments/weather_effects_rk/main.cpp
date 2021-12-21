#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"
#include "opengl_debug.h"

#include "plane_model.h"
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
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObjects();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);
void drawPlane(glm::mat4 model);

// screen settings
// ---------------
unsigned int screenWidth = 1080;
unsigned int screenHeight = 1080;
float loopInterval = 0.f;
bool renderAsParticle = true;

glm::vec3 gravityOffset = glm::vec3(0);
glm::vec3 gravityVelocity = glm::vec3(0.f, -1.f, 0.f);
glm::vec3 windOffset = glm::vec3(0);
glm::vec3 windVelocity = glm::vec3(0.2f, 0.f, 0.2f);
float distanceToCube = 10;
float currentTime = 0;
float particleScale = 3.f;

struct Camera {
    glm::vec3 forward = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 position = glm::vec3(0.f, 1.6f, 0.f);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 previousMvpMatrix = glm::mat4(1.0);
    float moveSpeed = 0.15f;
    float rotationGain = 30.0f;

    glm::mat4 getViewProjectionMatrix()
    {
        glm::mat4 viewMatrix = glm::lookAt(position, position + forward, glm::vec3(0,1,0));
        return (projectionMatrix * viewMatrix);
    }
};
Camera camera;

struct Particle {
    glm::vec3 position;
};

struct ParticleSystem {
    float boxSize = 2;
    unsigned int VAO{};
    unsigned int VBO{};
    int particleCount = 12000;
    int attributeCount = 3;
    std::vector<float> particlesDataBuffer = std::vector<float>(particleCount*attributeCount);
    Shader* program{};

    void generateParticles()
    {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed
        std::uniform_real_distribution<> distr(-boxSize/2, boxSize/2);

        for(int i = 0; i < particleCount*attributeCount; i+=attributeCount*2)
        {
            glm::vec3 position = glm::vec3(distr(gen), distr(gen), distr(gen));

            particlesDataBuffer[i + 0] = position.x;
            particlesDataBuffer[i + 1] = position.y;
            particlesDataBuffer[i + 2] = position.z;

            particlesDataBuffer[i + 3] = position.x;
            particlesDataBuffer[i + 4] = position.y;
            particlesDataBuffer[i + 5] = position.z;
        }

        glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(Particle), &particlesDataBuffer[0], GL_DYNAMIC_DRAW);
    }

    void init()
    {
        program = new Shader("shaders/particle.vert", "shaders/particle.frag");
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        for(float & i : particlesDataBuffer) i = 0.0f;

        glBufferData(GL_ARRAY_BUFFER, particleCount * sizeof(Particle), &particlesDataBuffer[0], GL_DYNAMIC_DRAW);
        bindParticleAttributes();
    }

    void bindParticleAttributes() {
        int posSize = 3; // each position consists of x,y,z
        GLuint vertexLocation = glGetAttribLocation(program->ID, "particlePosition");
        glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
        glEnableVertexAttribArray(vertexLocation);
    }

    void drawParticles()
    {
        glm::vec3 cameraPosition = camera.position;
        program->setVec3("cameraPosition", cameraPosition);

        glm::mat4 mvpMatrix = camera.getViewProjectionMatrix();
        program->setMat4("mvpMatrix", mvpMatrix);
        program->setMat4("previousMvpMatrix", camera.previousMvpMatrix);

        glm::vec3 forwardOffset = camera.forward;
        program->setVec3("forwardOffset", forwardOffset);
        program->setVec3("windVelocity", windVelocity);
        program->setVec3("gravityVelocity", gravityVelocity);

        gravityOffset += loopInterval * gravityVelocity;
        windOffset += loopInterval * windVelocity;

        glm::vec3 offsets = gravityOffset + windOffset;
        offsets -= cameraPosition + forwardOffset + boxSize/2;
        offsets = glm::mod(offsets, boxSize);
        program->setVec3("in_Offsets", offsets);

        program->setFloat("boxSize", boxSize);
        program->setBool("renderAsParticle", renderAsParticle);
        program->setFloat("particleScale", particleScale);

        glBindVertexArray(VAO);
        if (renderAsParticle)
        {
            glDisable(GL_BLEND);
            glDrawArrays(GL_POINTS, 0, particleCount);
        }
        else
        {
            glEnable(GL_BLEND);
            glDrawArrays(GL_LINES, 0, particleCount);
            glDisable(GL_BLEND);
        }
        glBindVertexArray(0);
        camera.previousMvpMatrix = mvpMatrix;
    }
};


// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject floorObj;
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;
Shader* shaderProgram;
ParticleSystem particleSystem;

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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Exercise 5.2", nullptr, nullptr);
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
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    // render loop
    // -----------
    // render every loopInterval seconds
    loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawObjects();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete shaderProgram;
    delete particleSystem.program;
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void drawObjects(){
    shaderProgram->use();

    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);
    glm::mat4 viewProjection = camera.getViewProjectionMatrix();

    // draw floor (the floor was built so that it does not need to be transformed)
    shaderProgram->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different locations and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);

    drawPlane(viewProjection * glm::translate(-2.0f, .5f, 2.0f) * glm::rotateX(glm::quarter_pi<float>()) * scale);
    drawPlane(viewProjection * glm::translate(2.0f, .5f, -2.0f) * glm::rotateX(glm::quarter_pi<float>() * 3.f) * scale);

    particleSystem.program->use();
    particleSystem.drawParticles();
}

void drawCube(glm::mat4 model){
    // draw object
    shaderProgram->setMat4("model", model);
    cube.drawSceneObject();
}

void drawPlane(glm::mat4 model){
    // draw plane body and right wing
    shaderProgram->setMat4("model", model);
    planeBody.drawSceneObject();
    planeWing.drawSceneObject();

    // propeller,
    glm::mat4 propeller = model * glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgram->setMat4("model", propeller);
    planePropeller.drawSceneObject();

    // right wing back,
    glm::mat4 wingRightBack = model * glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingRightBack);
    planeWing.drawSceneObject();

    // left wing,
    glm::mat4 wingLeft = model * glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgram->setMat4("model", wingLeft);
    planeWing.drawSceneObject();

    // left wing back,
    glm::mat4 wingLeftBack =  model *  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingLeftBack);
    planeWing.drawSceneObject();
}

void setup(){
    // initialize shaders
    shaderProgram = new Shader("shaders/default.vert", "shaders/default.frag");

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // load plane meshes into openGL
    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    planePropeller.VAO = createVertexArray(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();

    particleSystem.init();
    particleSystem.generateParticles();
}


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(shaderProgram->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(shaderProgram->ID, "color");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // creates and bind the EBO
    createElementArrayBuffer(indices);

    return VAO;
}

unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

    return VBO;
}

unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

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
    // get cursor position and scale it down to a smaller range
    int screenW, screenH;
    glfwGetWindowSize(window, &screenW, &screenH);
    glm::vec2 cursorPosition(0.0f);
    cursorInRange(posX, posY, screenW, screenH, -1.0f, 1.0f, cursorPosition.x, cursorPosition.y);

    // initialize with first value so that there is no jump at startup
    static glm::vec2 lastCursorPosition = cursorPosition;

    // compute the cursor position change
    glm::vec2 positionDiff = cursorPosition - lastCursorPosition;

    static float rotationAroundVertical = 0;
    static float rotationAroundLateral = 0;

    // require a minimum threshold to rotate
    if (glm::dot(positionDiff, positionDiff) > 1e-5f){
        camera.forward = glm::vec3 (0,0,-1);
        // rotate the forward vector around the Y axis, notices that w is set to 0 since it is a vector
        rotationAroundVertical += glm::radians(-positionDiff.x * camera.rotationGain);
        camera.forward = glm::rotateY(rotationAroundVertical) * glm::vec4(camera.forward, 0.0f);

        // rotate the forward vector around the lateral axis
        rotationAroundLateral +=  glm::radians(positionDiff.y * camera.rotationGain);
        // we need to clamp the range of the rotation, otherwise forward and Y axes get parallel
        rotationAroundLateral = glm::clamp(rotationAroundLateral, -glm::half_pi<float>() * 0.9f, glm::half_pi<float>() * 0.9f);

        glm::vec3 lateralAxis = glm::cross(camera.forward, glm::vec3(0, 1,0));

        camera.forward = glm::rotate(rotationAroundLateral, lateralAxis) * glm::vec4(camera.forward, 0);

        lastCursorPosition = cursorPosition;
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // TODO move the camera position based on keys pressed (use either WASD or the arrow keys)
    glm::vec3 forwardInXZ = glm::normalize(glm::vec3(camera.forward.x, 0, camera.forward.z));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        camera.position += forwardInXZ * camera.moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        camera.position -= forwardInXZ * camera.moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        camera.position -= glm::cross(forwardInXZ, glm::vec3(0, 1, 0)) * camera.moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        camera.position += glm::cross(forwardInXZ, glm::vec3(0, 1, 0)) * camera.moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){
        renderAsParticle = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
        renderAsParticle = false;
    }
    // snow
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
        particleScale = 3;
        gravityVelocity = glm::vec3(0.0f, -0.4f, 0.0f);
        windVelocity = glm::vec3(-0.1f, 0.f, -0.1f);
    }
    // rain
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
        particleScale = 1;
        gravityVelocity = glm::vec3(0.0f, -1.0f, 0.0f);
        windVelocity = glm::vec3(0.2f, 0.f, 0.2f);
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    camera.projectionMatrix = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}