#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"
#include "primitives.h"

#include "Camera.h"

// Constants
const int INSTANCES = 5;

// structure to hold render info
// -----------------------------
struct SceneObject {
    unsigned int VAO;
    unsigned int vertexCount;
    void drawSceneObject() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};

// forward declaration
struct WeatherSystem;

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObjects(glm::mat4 viewProjection);
void drawRain(glm::mat4 viewProj);
void drawSnow(glm::mat4 viewProj);
void drawWeather(glm::mat4 viewProj);
glm::mat4 getViewProjection();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
Camera* camera;
SceneObject cube;
SceneObject floorObj;

//! Scene object holding the weather particles
std::vector<WeatherSystem*> snowWeatherInstances;
std::vector<WeatherSystem*> rainWeatherInstances;
Shader* geometryShader;
Shader* rainShader;
Shader* snowShader;

// global variables used for control
// ---------------------------------
//! Random
std::random_device rd;
std::default_random_engine eng(rd());

//! Time
float currentTime, deltaTime = 0.f, lastFrame = 0.f;

//! The Box
float boxSize = 50.f;

//! Other useful stuff
bool toggleRain = true;
glm::mat4 prevViewProj; // previous view projection matrix

/* Struct that takes care of rendering an instance of weather effects */
struct WeatherSystem {
    static const unsigned int ATTR_SIZE = 3; // each vertex has 3 attributes, XYZ position

    std::shared_ptr<Shader> shader; // pointer to the correct shader
    unsigned int VAO{}; // self-explanatory
    unsigned int VBO{}; // self-explanatory
    unsigned int particleCount = 0; // total amount of particles
    unsigned int vertexCount = 0; // total amount of vertices
    unsigned int vertsPerParticle = 1; // vertices used to draw each particle

    // offsets and such formalities
    float particleSize = 10.0f; // size of particles (size of snow, length of rain streaks)
    float gravDelta = 2.0f; // gravity multiplier
    float windDelta = 1.0f; // wind multiplier
    glm::vec3 gravityOffset = glm::vec3(0); // gravity offset
    glm::vec3 windOffset = glm::vec3(0); // wind offset
    glm::vec3 randomOffset = glm::vec3(0); // random offset

    /* Constructor, takes shader, number of particles to render and amount of vertices per particle as parameters
     * and then initializes the rest of the struct. */
    WeatherSystem(Shader *part_shader, unsigned int particleCount, unsigned int vertsPerParticle)
    : particleCount(particleCount), shader(part_shader), vertsPerParticle(vertsPerParticle) {
        // set up buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // instance specific offsets and deltas
        static std::uniform_real_distribution<float> size(10.0f, 40.0f);
        static std::uniform_real_distribution<float> pos(-boxSize/2, boxSize/2);
        randomOffset = glm::vec3(pos(eng), pos(eng), pos(eng));
        particleSize = size(eng);
        gravDelta = 2.0f * particleSize/10;
        windDelta = 1.0f * 10/particleSize;

        // calculate amount of vertices to store in memory
        vertexCount = particleCount * vertsPerParticle;
        // initialize vertex buffer, set all values to 0
        std::vector<float> data(vertexCount * ATTR_SIZE);
        for (float & i : data)
            i = 0.0f;
        // allocate at openGL controlled memory
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_DYNAMIC_DRAW);
        GLuint vertexLocation = glGetAttribLocation(shader->ID, "pos");
        glEnableVertexAttribArray(vertexLocation);
        glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, ATTR_SIZE * sizeof(GLfloat), nullptr);
    }

    /* Initialize all the particles for the instance, this function is quite self-explanatory; each vertex of the same
     * particle is initialized with the same starting position. */
    void initParticles() const {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        std::uniform_real_distribution<float> pos(-boxSize/2, boxSize/2);
        std::vector<float> particles;
        for(unsigned int i = 0; i < particleCount; i++) {
            float x = pos(eng), y = pos(eng), z = pos(eng);
            for(unsigned int j = 0; j < vertsPerParticle; j++) {
                particles.insert(particles.end(), {x, y, z}); // insert random XYZ position
            }
        }
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(GLfloat), &particles[0], GL_DYNAMIC_DRAW);
    }

    /* Make the correct draw call for the particles */
    void drawParticles() const {
        glBindVertexArray(VAO);
        switch (vertsPerParticle) {
            case 1:
                glDrawArrays(GL_POINTS, 0, vertexCount);
                break;
            case 2:
                glDrawArrays(GL_LINES, 0, vertexCount);
                break;
            default:
                std::cerr << "Draw method not defined for particles with " << vertsPerParticle << " vertices." <<std::endl;
        }
    }

    /* Simulate one cycle of weather to update gravity and wind offsets */
    void simulate() {
        // self-explanatory
        gravityOffset -= glm::vec3(0, gravDelta * deltaTime, 0);
        // to add some variety the wind makes a circle :D
        windOffset += glm::vec3(sin(currentTime + randomOffset.x)/10, 0, cos(currentTime + randomOffset.z)/10) * windDelta;
    }
};

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // onit camera stuff
    camera = new Camera(glm::vec3(.0f, 1.6f, 0.0f));
    prevViewProj = camera->getViewMatrix();

    // setup mesh objects
    // ---------------------------------------
    setup();

    // set up the z-buffer
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system);
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    // NEW!
    // enable built in variable gl_PointSize in the vertex shader
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 1.0f / 60.0f;
    auto begin = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(window);

        glClearColor(0.02f, 0.01f, 0.2f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        geometryShader->use();
        auto viewProjection = getViewProjection();
        drawObjects(viewProjection);
        drawWeather(viewProjection);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete geometryShader;
    delete rainShader;
    delete snowShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void setup() {
    // initialize shaders
    geometryShader = new Shader("shaders/geometry.vert", "shaders/geometry.frag");
    rainShader = new Shader("shaders/rain.vert", "shaders/rain.frag");
    snowShader = new Shader("shaders/snow.vert", "shaders/snow.frag");

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // set up rain weather instances
    rainShader->use();
    rainShader->setFloat("boxSize", boxSize);
    for(unsigned int i = 0; i < INSTANCES; i++) {
        auto instance = new WeatherSystem(rainShader, (unsigned int) 1000/INSTANCES, 2);
        instance->gravDelta *= 10;
        instance->initParticles();
        rainWeatherInstances.push_back(instance);
    }

    // set up snow weather instances
    snowShader->use();
    snowShader->setFloat("boxSize", boxSize);
    for(unsigned int i = 0; i < INSTANCES; i++) {
        auto instance = new WeatherSystem(snowShader, (unsigned int) 5000/INSTANCES, 1);
        instance->initParticles();
        snowWeatherInstances.push_back(instance);
    }

    prevViewProj = getViewProjection();
}

/* simple function to get the view projection matrix */
glm::mat4 getViewProjection() {
    glm::mat4 projection = glm::perspectiveFov(glm::radians(90.0f), (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = camera->getViewMatrix();
   return projection * view;
}

void drawObjects(glm::mat4 viewProjection){
    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);

    // NEW!
    // update the camera pose and projection, and compose the two into the viewProjection with a matrix multiplication
    // projection * view = world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order (projection * view), we could read
    // perspective_projection_from_view <- view_from_world


    // draw floor (the floor was built so that it does not need to be transformed)
    geometryShader->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different locations and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);
}

void drawWeather(glm::mat4 viewProj) {
    if(toggleRain)
        drawRain(viewProj);
    else
        drawSnow(viewProj);
    prevViewProj = viewProj;
}

void drawRain(glm::mat4 viewProj) {
    for(const auto& instance : rainWeatherInstances) {
        instance->shader->use(); // start using the correct shader
        instance->simulate(); // update gravity & wind offsets
        auto fwdOffset = camera->forward * boxSize / 2.f; // update forward offset
        auto offsets = instance->gravityOffset + instance->windOffset + instance->randomOffset; // sum them all up
        offsets -= camera->position + fwdOffset + boxSize / 2.f; // factor in camera position and fwd offset
        offsets = glm::mod(offsets, boxSize); // constrain them inside the boxSize
        // set up uniforms
        instance->shader->setMat4("viewProj", viewProj);
        instance->shader->setMat4("viewProjPrev", prevViewProj);
        instance->shader->setVec3("cameraPos", camera->position);
        instance->shader->setVec3("forwardOffset", fwdOffset);
        instance->shader->setVec3("offsets", offsets);
        instance->shader->setVec3("velocity", glm::vec3(0.0f, 1.0f, 0.0f) * instance->gravDelta + glm::normalize(instance->windOffset));
        instance->shader->setFloat("heightScale", 0.01f * instance->particleSize/10);
        instance->drawParticles();
    }
}

void drawSnow(glm::mat4 viewProj) {
    for(const auto& instance : snowWeatherInstances) {
        instance->shader->use(); // start using the correct shader
        instance->simulate(); // update gravity & wind offsets
        auto fwdOffset = camera->forward * boxSize / 2.f; // update forward offset
        auto offsets = instance->gravityOffset + instance->windOffset + instance->randomOffset; // sum them all up
        offsets -= camera->position + fwdOffset + boxSize / 2.f; // factor in camera position and fwd offset
        offsets = glm::mod(offsets, boxSize); // constrain them inside the boxSize
        // set up uniforms
        instance->shader->setMat4("viewProj", viewProj);
        instance->shader->setVec3("cameraPos", camera->position);
        instance->shader->setVec3("forwardOffset", fwdOffset);
        instance->shader->setVec3("offsets", offsets);
        instance->shader->setFloat("maxSize", instance->particleSize);
        instance->drawParticles();
    }
}


void drawCube(glm::mat4 model){
    // draw object
    geometryShader->setMat4("model", model);
    cube.drawSceneObject();
}

unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(geometryShader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(geometryShader->ID, "color");
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
    static auto lastCoord = glm::vec2(posX, posY);
    auto offset = glm::vec2(posX - lastCoord.x, (posY - lastCoord.y) * -1.f);
    lastCoord = glm::vec2(posX, posY);
    // calc camera direction
    camera->processMouseInput(offset.x, offset.y);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera->processKeyInput(Direction::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera->processKeyInput(Direction::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera->processKeyInput(Direction::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera->processKeyInput(Direction::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        toggleRain = true;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        toggleRain = false;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}