#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "shader.h"
#include "glmutils.h"
#include "primitives.h"

//Solid Mesh Objects
struct SceneObject {
	unsigned int VAO;
	unsigned int vertexCount;
	void drawSceneObject() const {
		glDisable(GL_BLEND);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
	}
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);

void setup(); //Setup Mesh Objects
void setupParticles(); //Setup Particles Objects
void drawObjects(); //Render Declared Objects

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);

void drawCube(glm::mat4 model); //Draw Cube Object
void drawParticles(glm::mat4 model); //Draw Particles Objects

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
//SceneObjects - Solid Geometry 
SceneObject cube;
SceneObject floorObj;

//Shader programs in use
Shader* shaderProgram;
Shader* shaderProgramParticle;

// global variables used for control
// ---------------------------------
float currentTime, currentAngleX, currentAngleY, cursorPosPrevX = 0.0f, cursorPosPrevY = 0.0f;
float cameraMoveSpeed = 0.15f, rotationGain = 10.0f, dt = 0.05f, gravitySpeed = 0.045f, windSpeed = 0.006f;
int numOfParticles = 8000;
float lineScaleFactor = 0.010f; //0.010f with preset of boxSize = 2.0f 

glm::vec3 camPosition(0.0f, 1.6f, 10.0f); //camera position
glm::vec3 camFront(0.0f, 0.0f, -1.0f); //camera forward
glm::vec3 camUp(0.0f, 1.0f, 0.0f); //camera up
glm::vec3 camDirection = camFront; //direction of camera - Initial

glm::vec3 gravityOffset(0.0, -0.025f, 0.0);
glm::vec3 windDirection(0.025f, 0.0, 0.0);
glm::vec3 randomOffset(0.0, 0.0, 0.0);
glm::mat4 previousModel(1.0);
float boxSize = 2.0f;
glm::vec3 offsets(0.0); //offset position over time.

bool isLineRendering = true;

std::vector<float> particleData(numOfParticles * 6);
unsigned int VAO, VBO;


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
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Weather Effects", NULL, NULL);
	if (window == NULL){
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
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//Init Shaders
	shaderProgram = new Shader("shader.vert", "shader.frag");
	shaderProgramParticle = new Shader("particleShader.vert", "particleShader.frag");
	
	//Setup Mesh Objects
	setup();
	
	//Setup Particles
	setupParticles();

	//Points Rendering Flag enabled
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	
	// set up the z-buffer
	glDepthRange(-1, 1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
	glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
	glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);


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
		currentTime = appTime.count();
	
		//Movement and switches
		processInput(window);

		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Render Mesh and Particles

		drawObjects();

		glfwSwapBuffers(window);
		glfwPollEvents();

		// control render loop frequency
		std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now() - frameStart;
		while (loopInterval > elapsed.count()) {
			elapsed = std::chrono::high_resolution_clock::now() - frameStart;
		}
	}

	delete shaderProgram;
	delete shaderProgramParticle;

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

void drawObjects() {
	//Mesh Render
	shaderProgram->use(); //shader for solid geometries
	glm::mat4 scale = glm::scale(1.f, 1.f, 1.f); //scale of objects (except floor)

	glm::mat4 view = glm::lookAt(camPosition, camPosition + camDirection, camUp); //view matrix
	glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f); //projection matrix

	//Draw Floor (no TRS transformation required) only the MVP where model remains the same.
	shaderProgram->setMat4("model", projection * view);
	floorObj.drawSceneObject();

	//Draw 2 cubes
	drawCube(projection * view * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
	drawCube(projection * view * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);

	//Particles Render
	shaderProgramParticle->use();
	drawParticles(projection * view);
}

void drawParticles(glm::mat4 model) {
	int isParticleID = glGetUniformLocation(shaderProgramParticle->ID, "isParticle");
	
	if (isLineRendering) {
		glUniform1i(isParticleID, 0);
	}
	else {
		glUniform1i(isParticleID, 1);
	}


	//Draw Particles
	glm::vec3 forwardOffset = camDirection;

	int particleGravityID = glGetUniformLocation(shaderProgramParticle->ID, "particleGravity");
	glUniform3f(particleGravityID, gravityOffset.x, gravityOffset.y, gravityOffset.z);

	int particleVelocityID = glGetUniformLocation(shaderProgramParticle->ID, "particleVelocity");
	glUniform3f(particleVelocityID, windDirection.x, windDirection.y, windDirection.z);

	int randomID = glGetUniformLocation(shaderProgramParticle->ID, "particleRandom");
	glUniform3f(randomID, randomOffset.x, randomOffset.y, randomOffset.z);

	int scaleFactorID = glGetUniformLocation(shaderProgramParticle->ID, "lineScaleFactor");
	glUniform1f(scaleFactorID, lineScaleFactor);

	//Gravity and Wind Control Limiter
	gravityOffset.y -= gravitySpeed;
	windDirection.x -= windSpeed;

	int cameraPosID = glGetUniformLocation(shaderProgramParticle->ID, "cameraPos");
	int boxSizeID = glGetUniformLocation(shaderProgramParticle->ID, "boxSize");
	int offsetID = glGetUniformLocation(shaderProgramParticle->ID, "offsets");
	int camDirectionID = glGetUniformLocation(shaderProgramParticle->ID, "camDirection");

	//Current MVP
	glm::mat4 view = glm::lookAt(camPosition, camPosition + camDirection, camUp); //view matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f); //projection matrix
	shaderProgramParticle->setMat4("particleModel", projection * view);//current model 

	//Previous MVP
	shaderProgramParticle->setMat4("particleModelPrev", previousModel); //previous model

	glm::vec3 randomOffset = glm::vec3((rand() / RAND_MAX) / 1.0f * dt, 0.0f, (rand() / RAND_MAX) / 1.0f * dt);
			
	//pass the Camera direction as parameter to shader.
	glUniform3f(camDirectionID, camDirection.x, camDirection.y, camDirection.z);

	//Calculate offset
	offsets = gravityOffset + windDirection + randomOffset;
	offsets -= camPosition + forwardOffset + glm::vec3(boxSize, boxSize, boxSize)/2.0f;
	offsets = glm::mod(offsets, glm::vec3(boxSize, boxSize, boxSize));

	//pass the BoxSize as parameter to shader.
	glUniform1f(boxSizeID, boxSize);

	//Use only to debug what the effect looks like from 3rd person view. --DEBUG ONLY
	//glm::vec3 normCameraPos = glm::normalize(camPosition);
	//glUniform3f(cameraPosID, normCameraPos.x, normCameraPos.y, normCameraPos.z);

	//pass cameraPosition as parameter to shader.
	glUniform3f(cameraPosID, camPosition.x, camPosition.y, camPosition.z);

	//pass offsets as parameter to shader.
	glUniform3f(offsetID, offsets.x, offsets.y, offsets.z);

	//Call Buffer
	glBindVertexArray(VAO);

	//Draw Particle	
	if (isLineRendering) {
		glEnable(GL_BLEND);
		glDrawArrays(GL_LINES, 0, numOfParticles);

	}
	else {
		glDisable(GL_BLEND);
		glDrawArrays(GL_POINTS, 0, numOfParticles);
	}

	//Store the previous model
	previousModel = model; // appends the previous model
}

void drawCube(glm::mat4 model) {
	// draw object
	shaderProgram->setMat4("model", model);
	cube.drawSceneObject();
}

void setup() {
	// load floor mesh into openGL
	floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
	floorObj.vertexCount = floorIndices.size();

	// load cube mesh into openGL
	cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
	cube.vertexCount = cubeIndices.size();
}

void setupParticles() {
	float mirroredRandomX, mirroredRandomY, mirroredRandomZ;
	float randomX, randomY, randomZ;
	
	//generate buffers for VAO,VBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Bind VAO , Bind VBO
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	bool flip = false;

	const float MIN_RAND = -boxSize / 2.0f, MAX_RAND = boxSize / 2.0f;
	const float range = MAX_RAND - MIN_RAND;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

	//CREATE RANDOM POSITIONS 
	for (int i = 0; i < numOfParticles * 6; i += 6) {
		if (!flip) {
			//Setting Range for x,y,z point coords in world space.
			randomX = range * ((((float)rand()) / (float)RAND_MAX)) + MIN_RAND;
			randomY = range * ((((float)rand()) / (float)RAND_MAX)) + MIN_RAND;
			randomZ = range * ((((float)rand()) / (float)RAND_MAX)) + MIN_RAND;

			particleData[i + 0] = randomX; //posX
			particleData[i + 1] = randomY; //posY
			particleData[i + 2] = randomZ; //posZ
			particleData[i + 3] = color.r; //R
			particleData[i + 4] = color.g; //G
			particleData[i + 5] = color.b; //B

			mirroredRandomX = randomX;
			mirroredRandomY = randomY;
			mirroredRandomZ = randomZ;
			flip = true;
		}
		else {
			randomX = mirroredRandomX; //-0.5f;
			randomY = mirroredRandomY; //-0.5f;
			randomZ = mirroredRandomZ; //-0.5f;
			particleData[i + 0] = randomX; //posX
			particleData[i + 1] = randomY; //posY
			particleData[i + 2] = randomZ; //posZ
			particleData[i + 3] = color.r; //R
			particleData[i + 4] = color.g; //G
			particleData[i + 5] = color.b; //B
			flip = false;

        }

	}
		
	int posSize = 3; // each position has x,y,z
	int colSize = 3; // each color has r,g,b

	glBufferData(GL_ARRAY_BUFFER, numOfParticles * 6.0f * sizeof(float), &particleData[0], GL_STATIC_DRAW);

	GLuint particlePos = glGetAttribLocation(shaderProgramParticle->ID, "particlePos");
	GLuint particleCol = glGetAttribLocation(shaderProgramParticle->ID, "particleColor");

	glEnableVertexAttribArray(particlePos);
	glVertexAttribPointer(particlePos, posSize, GL_FLOAT, GL_FALSE, 6.0f * sizeof(float), (void*)(0 * sizeof(float)));

	glEnableVertexAttribArray(particleCol);
	glVertexAttribPointer(particleCol, colSize, GL_FLOAT, GL_FALSE, 6.0f * sizeof(float), (void*)(3 * sizeof(float)));	
}

unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices) {
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

unsigned int createArrayBuffer(const std::vector<float> &array) {
	unsigned int VBO;
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

	return VBO;
}

unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array) {
	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

	return EBO;
}

void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, double &x, double &y) {
	float sum = max - min;
	float xInRange = (float)screenX / (float)screenW * sum - sum / 2.0f;
	float yInRange = (float)screenY / (float)screenH * sum - sum / 2.0f;
	x = xInRange;
	y = -yInRange; // flip screen space y axis
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY) {
	int screenW, screenH; //window resolution
	double screenX, screenY; //screen coordinate holders.
	float mouseXSens = 0.25f, mouseYSens = 0.25f;

	glfwGetWindowSize(window, &screenW, &screenH); //get window size
	glfwGetCursorPos(window, &screenX, &screenY); //get screen coordinates (unbounded)
	
	cursorInRange(screenX, screenY, screenW, screenH, -2.0f, 2.0f, posX, posY);
	
	float tempAngleX = currentAngleX;
	float tempAngleY = currentAngleY;

	//Rotate Up (X-Axis)
	if (screenY < -0.5) {
		currentAngleX = glm::mix(tempAngleX, tempAngleX + glm::radians(rotationGain) * glm::abs((float)(posY - cursorPosPrevY)) , 1.0f);
		if (currentAngleX > glm::radians(89.9f)) currentAngleX = glm::radians(89.9f);
	}

	//Rotate Down (X-Axis)
	if (screenY > 0.5f) {
		currentAngleX = glm::mix(tempAngleX,tempAngleX - glm::radians(rotationGain) * glm::abs((float)(posY - cursorPosPrevY)) , 1.0f);
		if (currentAngleX < glm::radians(-89.9f)) currentAngleX = glm::radians(-89.9f);
	}

	//Rotate Left (Y-Axis)
	if (screenX < -0.5f) currentAngleY = glm::mix(tempAngleY,tempAngleY - glm::radians(rotationGain) * glm::abs((float)(posX - cursorPosPrevX)) , 1.0f);
	
	//Rotate Right (Y-Axis)
	if (screenX > 0.5f) currentAngleY = glm::mix(tempAngleY, tempAngleY + glm::radians(rotationGain) * glm::abs((float)(posX - cursorPosPrevX)) , 1.0f);
	
	//Store previous values for cursor pos X,Y
	cursorPosPrevX = posX;
	cursorPosPrevY = posY;

	//Calculate Direction
	camDirection.x = glm::cos(currentAngleY) * glm::cos(currentAngleX);
	camDirection.y = glm::sin(currentAngleX);
	camDirection.z = glm::sin(currentAngleY) * glm::cos(currentAngleX);

	glfwSetCursorPos(window, 0.0f, 0.0f); //reset cursor position to avoid exceeding input bounds 
}

void processInput(GLFWwindow *window) {
	//Escape Application
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

	//DEBUG	
	//Increase speed 
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) gravitySpeed += 0.001f;
	
	//Decrease speed
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) gravitySpeed -= 0.001f;


	//Wind Direction (West - East)
	//WEST
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) windSpeed -= 0.001f;

	//EAST
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) windSpeed += 0.001f;
	

	//Enable Line Rendering
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) isLineRendering = true;

	//Enable Particle Rendering
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) isLineRendering = false;
	

	//Rain Type of Particles
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		int particleScaleID = glGetUniformLocation(shaderProgramParticle->ID, "particlePointSize");
		glUniform1f(particleScaleID, 0.0f);
		int particleVelocityID = glGetUniformLocation(shaderProgramParticle->ID, "particleVelocity");
		glUniform3f(particleVelocityID, windDirection.x, windDirection.y, windDirection.z);

		gravitySpeed = 0.045f;
	}

	//Snow Type of Particles
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		int particleScaleID = glGetUniformLocation(shaderProgramParticle->ID, "particlePointSize");
		glUniform1f(particleScaleID, 1.0f);
		int particleVelocityID = glGetUniformLocation(shaderProgramParticle->ID, "particleVelocity");
		glUniform3f(particleVelocityID, windDirection.x, windDirection.y, windDirection.z);
		gravitySpeed = 0.025f;
	}
	
	//Movement using WASD
	
	//Move Forward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPosition += glm::vec3(camDirection.x, 0.0f, camDirection.z) * 0.1f;
	
	//Move Backwards
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPosition -= glm::vec3(camDirection.x, 0.0f, camDirection.z) * 0.1f;
	
	//Move Left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPosition -= glm::normalize(glm::cross(glm::vec3(camDirection.x, 0.0f, camDirection.z), camUp)) * 0.1f;
	
	//Move Right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPosition += glm::normalize(glm::cross(glm::vec3(camDirection.x, 0.0f, camDirection.z), camUp)) * 0.1f;
}

//Updates the new window size on runtime.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}