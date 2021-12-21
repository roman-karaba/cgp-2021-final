#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"

#include "plane_model.h"
#include "primitives.h"
#include "Camera.h"

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

float lastX, lastY;

const unsigned int vertexBufferSize = 10000;    // # of particles

// TODO 2.2 update the number of attributes in a particle
const unsigned int particleSize = 3;            // particle attributes

const unsigned int sizeOfFloat = 4;             // bytes in a float
const float boxSize = 30;
unsigned int particleId = 0;                    // keep track of last particle to be updated
glm::vec3 gravityConst = glm::vec3(0, -1, 0);
glm::vec3 windConst = glm::vec3(0.5, 0, -0.5);
glm::vec3 gravityOffset = glm::vec3(0, 0, 0);
glm::vec3 windOffset = glm::vec3(0, 0, 0);
glm::vec3 randomOffset =glm::vec3(0, 0, 0);

std::vector<glm::vec3> gravityOffsets;
std::vector<glm::vec3> windOffsets;
std::vector<glm::vec3> randomOffsets;
std::vector<float> gravityVelocities;
std::vector<float> windVelocities;
int numberOfDraws = 1;

float deltaTime = 0.0f;
bool isPoint = true;

// For the particles
unsigned int VAO;
unsigned int VBO;



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
void drawParticles();
void emitParticle();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);
void createVertexBufferObjectForParticles();
void bindAttributesForParticles();



// global variables used for rendering
// -----------------------------------
// camera
Camera camera(glm::vec3(0.0f, 1.0f, 3));
SceneObject cube;
SceneObject floorObj;
std::vector<Shader> shaderPrograms;
Shader* shaderProgram;

// global variables used for control
// ---------------------------------
float currentTime;
glm::vec3 camForward(.0f, .0f, -1.0f);
glm::vec3 camPosition(.0f, 1.6f, 0.0f);
float linearSpeed = 0.15f, rotationGain = 30.0f;


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
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); // Enable point size to set the particle size based on the distance


    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        shaderProgram = &shaderPrograms[0];
        shaderProgram->use();
        drawObjects();

        shaderProgram = &shaderPrograms[1];
        shaderProgram->use();
        /*for (size_t i = 0; i < 20; i++)
        {
            emitParticle();
        }*/
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
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

    //delete shaderProgram;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void drawParticles() 
{
    
    glm::mat4 projection = glm::perspectiveFov(70.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();//glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    glm::mat4 viewProjection = projection * view;
    
    static glm::mat4 prevViewProj = viewProjection;

    // draw floor (the floor was built so that it does not need to be transformed)
    for (int i = 0; i<numberOfDraws; i++) {
        if(isPoint){
            gravityOffsets[i] += gravityConst * gravityVelocities[i] * deltaTime;
        }
        else{
            gravityOffsets[i] += gravityConst * gravityVelocities[i] * deltaTime * 10.0f;
        }
        windOffsets[i] += windConst * windVelocities[i]*deltaTime;
        
        //gravityOffset += gravityConst * deltaTime;
        //windOffset += windConst * deltaTime;
        
        glm::vec3 offsets = gravityOffsets[i] + windOffsets[i] + randomOffsets[i];
        //offsets += gravityConst;
        //std::cout<<offsets<<std::endl;
        offsets -= camera.Position + (camera.Front*(boxSize/2)) + boxSize/2;
        offsets = glm::mod(offsets, boxSize);
        std::cout<<offsets<<std::endl;
        //std::cout<<camera.Position<<std::endl;
        
        shaderProgram->setMat4("viewProj", viewProjection);
        shaderProgram->setVec3("offsets", offsets);
        shaderProgram->setVec3("cameraPos", camera.Position);
        shaderProgram->setVec3("forwardOffset", (camera.Front*(boxSize/2)));
        shaderProgram->setBool("isPoint", isPoint);
        

        glBindVertexArray(VAO);
        if(isPoint){
            
            glDrawArrays(GL_POINTS, 0, vertexBufferSize);
        }
        else{
            glm::vec3 velocity = -gravityConst * gravityVelocities[i] * 10.0f - windConst * windVelocities[i];
            shaderProgram->setVec3("velocity", velocity);
            shaderProgram->setFloat("heightSize", 0.1);
            shaderProgram->setMat4("prevViewProj", prevViewProj);
            glDrawArrays(GL_LINES, 0, vertexBufferSize);
            
        }
        
        prevViewProj = viewProjection;
    }
   
}


void drawObjects(){


    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);

    // NEW!
    // update the camera pose and projection, and compose the two into the viewProjection with a matrix multiplication
    // projection * view = world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order (projection * view), we could read
    // perspective_projection_from_view <- view_from_world
    glm::mat4 projection = glm::perspectiveFov(70.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();//glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    glm::mat4 viewProjection = projection * view;

    // draw floor (the floor was built so that it does not need to be transformed)
    shaderProgram->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different locations and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);
}


void drawCube(glm::mat4 model){
    // draw object
    shaderProgram->setMat4("model", model);
    cube.drawSceneObject();
}



void setup(){
    // initialize shaders
    shaderPrograms.push_back(Shader("shaders/shader.vert", "shaders/shader.frag"));
    shaderPrograms.push_back(Shader("shaders/particle.vert", "shaders/particle.frag"));
    shaderProgram = &shaderPrograms[0];
    
    float max_rand = (float)(RAND_MAX);
    randomOffset = glm::vec3(((float)(rand()) / max_rand) * 1,0,((float)(rand()) / max_rand) * 1);
    
    for (int i=0; i<numberOfDraws; i++) {
        gravityOffsets.push_back(glm::vec3(0,0,0));
        windOffsets.push_back(glm::vec3(0,0,0));
        randomOffsets.push_back(glm::vec3(((float)(rand()) / max_rand) * 1,0,((float)(rand()) / max_rand) * 1));
        
        gravityVelocities.push_back(.7f * 0.2f * (rand()%11));
        windVelocities.push_back(.7f * 0.2f * (rand()%11));
        
    }

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    createVertexBufferObjectForParticles();
}

// OBJECTS
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

// PARTICLES
void bindAttributesForParticles() {
    int posSize = 3; // each position has x,y,z
    GLuint vertexLocation = glGetAttribLocation(shaderProgram->ID, "pos");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE, particleSize * sizeOfFloat, 0);
}

void createVertexBufferObjectForParticles() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    float max_rand = (float)(RAND_MAX);
    
    if(isPoint){
        std::vector<float> data(vertexBufferSize *particleSize);
        for (unsigned int i = 0; i < data.size(); i++){
            
            data[i]=((float)(rand()) / max_rand) * 30;
            
        }
        
        std::cout<<data.size();
        // allocate at openGL controlled memory
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize * particleSize * sizeOfFloat , &data[0], GL_DYNAMIC_DRAW);
        bindAttributesForParticles();
    }
    else{
        std::vector<float> data(vertexBufferSize *particleSize);
        for (unsigned int i = 0; i<data.size()/2; i++) {
            float randX = ((float)(rand()) / max_rand) * 30;
            float randY = ((float)(rand()) / max_rand) * 30;
            float randZ = ((float)(rand()) / max_rand) * 30;
            
            data[i*6] = randX;
            data[i*6+1] = randY;
            data[i*6+2] = randZ;
            data[i*6+3] = randX;
            data[i*6+4] = randY;
            data[i*6+5] = randZ;
        }
        
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize * particleSize * sizeOfFloat , &data[0], GL_DYNAMIC_DRAW);
        bindAttributesForParticles();
    }
   


    //return VAO;
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
    // TODO - rotate the camera position based on mouse movements
    //  if you decide to use the lookAt function, make sure that the up vector and the
    //  vector from the camera position to the lookAt target are not collinear

    // get cursor position and scale it down to a smaller range
    int screenW, screenH;
    glfwGetWindowSize(window, &screenW, &screenH);
    glm::vec2 cursorPosition(0.0f);
    cursorInRange(posX, posY, screenW, screenH, -1.0f, 1.0f, cursorPosition.x, cursorPosition.y);

    // initialize with first value so that there is no jump at startup
    static glm::vec2 lastCursorPosition = cursorPosition;

    // compute the cursor position change
    glm::vec2 positionDiff = cursorPosition - lastCursorPosition;

    // require a minimum threshold to rotate
    if (glm::dot(positionDiff, positionDiff) > 1e-5f){
        camera.ProcessMouseMovement(positionDiff.x, positionDiff.y);
        lastCursorPosition = cursorPosition;
    }

}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
        isPoint = true;
        setup();
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
        isPoint = false;
        setup();
    }

    // TODO move the camera position based on keys pressed (use either WASD or the arrow keys)
    glm::vec3 forwardInXZ = glm::normalize(glm::vec3(camForward.x, 0, camForward.z));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    /*double xPos, yPos;
    int xScreen, yScreen;
    glfwGetCursorPos(window, &xPos, &yPos);
    glfwGetWindowSize(window, &xScreen, &yScreen);

    float xNdc = (float)xPos / (float)xScreen * 2.0f - 1.0f;
    float yNdc = (float)yPos / (float)yScreen * 2.0f - 1.0f;
    yNdc = -yNdc;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // compute velocity based on two consecutive updates
        float velocityX = xNdc - lastX;
        float velocityY = yNdc - lastY;
        float max_rand = (float)(RAND_MAX);
        // create 5 to 10 particles per frame
        int i = (int)((float)(rand()) / max_rand) * 5;
        for (; i < 10; i++) {
            // add some randomness to the movement parameters
            float offsetX = ((float)(rand()) / max_rand - .5f) * .1f;
            float offsetY = ((float)(rand()) / max_rand - .5f) * .1f;
            float offsetVelX = ((float)(rand()) / max_rand - .5f) * .1f;
            float offsetVelY = ((float)(rand()) / max_rand - .5f) * .1f;
            // create the particle
            emitParticle(xNdc + offsetX, yNdc + offsetY, velocityX + offsetVelX, velocityY + offsetVelY, currentTime);
        }
    }
    lastX = xNdc;
    lastY = yNdc;*/
}

/*void emitParticle() {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    float max_rand = (float)(RAND_MAX);
    glm::vec3 random = glm::vec3((int)((float)(rand()) / max_rand) * 5, 0, (int)((float)(rand()) / max_rand) * 5);

    glm::vec3 offsets = gravityConst + windConst + random;

    offsets -= camera.Position + boxSize / 2;


    offsets = glm::mod(offsets, boxSize);

    float data[particleSize];
    data[0] = 0;
    data[1] = 10,
    data[2] = 0,
    data[3] = offsets.x,
    data[4] = offsets.y;
    data[5] = offsets.z;
    data[6] = boxSize;
    data[7] = camera.Position.x;
    data[8] = camera.Position.y;
    data[9] = camera.Position.z;

    // TODO 2.2 , add velocity and timeOfBirth to the particle data



    // upload only parts of the buffer
    glBufferSubData(GL_ARRAY_BUFFER, particleId * particleSize * sizeOfFloat, particleSize * sizeOfFloat, data);
    particleId = (particleId + 1) % vertexBufferSize;
}
*/

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
