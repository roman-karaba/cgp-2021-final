#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"

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

struct ParticleObject {
    unsigned  int VAO;
    unsigned  int vertexBufferSize;
    bool isRain;
    void drawParticles() const {
        glBindVertexArray(VAO);
        if(isRain)
            glDrawArrays(GL_LINES, 0, vertexBufferSize);
        else
            glDrawArrays(GL_POINTS, 0, vertexBufferSize);
    }
};

// function declarations
// ---------------------
unsigned int createParticles(bool isLine);
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(Shader* curShader, const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObjects();
void drawParticles();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double offsetX, double offsetY);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject floorObj;
ParticleObject snowObj;
ParticleObject rainObj;
Shader* sceneShaderProgram;
Shader* snowShaderProgram;
Shader* rainShaderProgram;

// global variables used for control
// ---------------------------------
float currentTime, deltaTime;
glm::vec3 camForward(.0f, .0f, -1.0f);
glm::vec3 camPosition(.0f, 1.6f, 0.0f);
glm::vec3 camUp(.0f, 1.f, .0f);

float linearSpeed = 4.f, rotationGain = 30.0f, mouseSensitivity = 7.f;
float fov = 70.f;

unsigned int numberOfParticleDraws = 3;
unsigned int particlesCount = 10000;
float boxSize = 15.f;
bool renderRain = false;

const glm::vec3 gravityDir(.0f, -1.0f, .0f);
const glm::vec3 windDir = glm::normalize(glm::vec3(4.f, .0f, -3.f));

std::vector<glm::vec3> gravityOffsets;
std::vector<glm::vec3> windOffsets;
std::vector<glm::vec3> randomOffsets;

std::vector<float> gravityDeltas;
std::vector<float> windDeltas;
float rainGravityDelta = 10.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 5.2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::srand(std::time(0));

    // setup mesh objects
    // ---------------------------------------
    setup();

    // enable built in variable gl_PointSize in the vertex shader and alpha blending
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    // set up the z-buffer
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system);
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();
    currentTime = .0f;

    while (!glfwWindowShouldClose(window))
    {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        deltaTime = appTime.count() - currentTime;
        currentTime = appTime.count();

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShaderProgram->use();
        drawObjects();

        if(renderRain)
            rainShaderProgram->use();
        else
            snowShaderProgram->use();
        glEnable(GL_BLEND);
        drawParticles();
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete sceneShaderProgram;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void drawObjects(){
    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);
    glm::mat4 scaleSmall = glm::scale(.1f, .1f, .1f);

    // update the camera pose and projection, and compose the two into the viewProjection with a matrix multiplication
    // projection * view = world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order (projection * view), we could read
    // perspective_projection_from_view <- view_from_world
    glm::mat4 projection = glm::perspectiveFov(fov, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    glm::mat4 viewProjection = projection * view;

    // draw floor (the floor was built so that it does not need to be transformed)
    sceneShaderProgram->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw some cubes for orientation
    drawCube(viewProjection * glm::translate(5.0f, 1.f, 5.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-5.0f, 1.f, -5.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-4.0f, 1.f, 4.0f) * glm::rotateY(glm::radians(310.f)) * scale);
    drawCube(viewProjection * glm::translate(4.0f, 1.f, -4.0f) * glm::rotateY(glm::radians(70.f)) * scale);
    drawCube(viewProjection * glm::translate(.0f, 1.f, 10.0f) * glm::rotateY(glm::radians(130.f)) * scale);
    drawCube(viewProjection * glm::translate(.0f, 1.f, -10.0f) * glm::rotateY(glm::radians(80.f)) * scale);
    drawCube(viewProjection * glm::translate(10.0f, 1.f, .0f) * glm::rotateY(glm::radians(170.f)) * scale);
    drawCube(viewProjection * glm::translate(-10.0f, 1.f, .0f) * glm::rotateY(glm::radians(240.f)) * scale);
    //drawCube(viewProjection * glm::translate(camPosition + camForward) * scaleSmall);
}

void drawParticles() {
    glm::vec3 fwdOffset = camForward * boxSize * .5f;
    for(int i=0; i<numberOfParticleDraws; i++) {
        gravityOffsets[i] += renderRain ? rainGravityDelta * gravityDir * gravityDeltas[i] * deltaTime : gravityDir * gravityDeltas[i] * deltaTime;
        windOffsets[i] += windDir * windDeltas[i] * deltaTime;

        glm::vec3 offset = gravityOffsets[i] + windOffsets[i] + randomOffsets[i];
        offset -= camPosition + fwdOffset + boxSize * .5f;
        offset = glm::mod(offset, boxSize);

        glm::mat4 projection = glm::perspectiveFov(fov, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
        glm::mat4 view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
        glm::mat4 viewProjection = projection * view;
        static glm::mat4 oldViewProjection = viewProjection;

        if(renderRain) {
            rainShaderProgram->setVec3("offset", offset);
            rainShaderProgram->setVec3("velocity", -gravityDir * gravityDeltas[i] * rainGravityDelta - windDir * windDeltas[i]);
            rainShaderProgram->setVec3("camPosition", camPosition);
            rainShaderProgram->setVec3("fwdOffset", fwdOffset);
            rainShaderProgram->setMat4("viewproj", viewProjection);
            rainShaderProgram->setMat4("viewprojPrev", oldViewProjection);
            rainObj.drawParticles();
        }
        else {
            snowShaderProgram->setVec3("offset", offset);
            snowShaderProgram->setVec3("camPosition", camPosition);
            snowShaderProgram->setVec3("fwdOffset", fwdOffset);
            snowShaderProgram->setMat4("viewproj", viewProjection);
            snowObj.drawParticles();
        }
        oldViewProjection = viewProjection;
    }
}

void drawCube(glm::mat4 model){
    // draw object
    sceneShaderProgram->setMat4("model", model);
    cube.drawSceneObject();
}


void setup(){
    // initialize shaders
    sceneShaderProgram = new Shader("shaders/sceneShader.vert", "shaders/sceneShader.frag");
    snowShaderProgram = new Shader("shaders/snowShader.vert", "shaders/snowShader.frag");
    rainShaderProgram = new Shader("shaders/rainShader.vert", "shaders/rainShader.frag");

    // Set the boxSize in the shaders
    snowShaderProgram->use();
    snowShaderProgram->setFloat("boxSize", boxSize);
    rainShaderProgram->use();
    rainShaderProgram->setFloat("boxSize", boxSize);
    rainShaderProgram->setFloat("heightScale", 0.05f);

    // setup the offsets
    for(int i=0; i<numberOfParticleDraws; i++) {
        // gravity deltas
        gravityDeltas.push_back(.5f + 0.1f * (rand() % 11));
        gravityOffsets.emplace_back(glm::vec3(.0f));
        // wind deltas
        windDeltas.push_back(0.2f + 0.1f * (rand() % 11));
        windOffsets.emplace_back(glm::vec3(.0f));
        // random offsets
        float randX = (std::rand() % 101) / 100.0f;
        float randY = (std::rand() % 101) / 100.0f;
        float randZ = (std::rand() % 101) / 100.0f;
        randomOffsets.emplace_back(glm::vec3(randX, randY, randZ));
    }

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(sceneShaderProgram, floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(sceneShaderProgram, cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // load snow particles
    snowObj.VAO = createParticles(false);
    snowObj.vertexBufferSize = particlesCount;
    snowObj.isRain = false;

    // load rain particles
    rainObj.VAO = createParticles(true);
    rainObj.vertexBufferSize = particlesCount;
    rainObj.isRain = true;
}

unsigned int createParticles(bool isLine) {
    unsigned  int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    std::vector<float> particles;
    if(isLine) {
        particles = std::vector<float>(particlesCount * 3);
        for(unsigned int i = 0; i < particlesCount / 2; i++) {
            float randX = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
            float randY = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
            float randZ = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
            particles[i*6] = randX;
            particles[i*6+1] = randY;
            particles[i*6+2] = randZ;
            particles[i*6+3] = randX;
            particles[i*6+4] = randY;
            particles[i*6+5] = randZ;
        }
    }
    else {
        particles = std::vector<float>(particlesCount * 3);
        for(unsigned int i = 0; i < particlesCount; i++) {
            particles[i*3] = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
            particles[i*3+1] = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
            particles[i*3+2] = boxSize*((float)std::rand() / (float)RAND_MAX) + boxSize;
        }
    }

    createArrayBuffer(particles);
    int posAttributeLocation;
    if(isLine)
        posAttributeLocation = glGetAttribLocation(rainShaderProgram->ID, "pos");
    else
        posAttributeLocation = glGetAttribLocation(snowShaderProgram->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    return VAO;
}

unsigned int createVertexArray(Shader* curShader, const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(curShader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(curShader->ID, "color");
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

void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y){
    float sum = max - min;
    float xInRange = (float) screenX / (float) screenW * sum - sum/2.0f;
    float yInRange = (float) screenY / (float) screenH * sum - sum/2.0f;
    x = xInRange;
    y = -yInRange; // flip screen space y axis
}

void scroll_callback(GLFWwindow* window, double offsetX, double offsetY)
{
    fov -= (float)offsetY * deltaTime;
    fov = glm::clamp(fov, 1.f, 70.f);
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){
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
        camForward = glm::vec3 (0,0,-1);
        // rotate the forward vector around the Y axis, notices that w is set to 0 since it is a vector
        rotationAroundVertical += glm::radians(-positionDiff.x * rotationGain) * deltaTime * mouseSensitivity;
        camForward = glm::rotateY(rotationAroundVertical) * glm::vec4(camForward, 0.0f);

        // rotate the forward vector around the lateral axis
        rotationAroundLateral +=  glm::radians(positionDiff.y * rotationGain) * deltaTime * mouseSensitivity;
        // we need to clamp the range of the rotation, otherwise forward and Y axes get parallel
        rotationAroundLateral = glm::clamp(rotationAroundLateral, -glm::half_pi<float>() * 0.9f, glm::half_pi<float>() * 0.9f);

        glm::vec3 lateralAxis = glm::cross(camForward, glm::vec3(0, 1,0));

        camForward = glm::rotate(rotationAroundLateral, lateralAxis) * glm::vec4(camForward, 0);

        lastCursorPosition = cursorPosition;
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        renderRain = false;
    if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        renderRain = true;

    glm::vec3 normalFwd = glm::normalize(glm::vec3(camForward.x, .0f, camForward.z));

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPosition += linearSpeed * normalFwd * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPosition -= linearSpeed * normalFwd * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPosition += linearSpeed * glm::normalize(glm::cross(normalFwd, camUp)) * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPosition -= linearSpeed * glm::normalize(glm::cross(normalFwd, camUp)) * deltaTime;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}