//
// Created by kb on 12/11/2021.
//

#ifndef ITU_GRAPHICS_PROGRAMMING_CAMERA_H
#define ITU_GRAPHICS_PROGRAMMING_CAMERA_H




#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// options for camera movement
enum Direction {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera {
    public:
        glm::vec3 position{};
        glm::vec3 forward{};
        glm::vec3 up{};
        glm::vec3 right{};
        glm::vec3 worldUp{};

        float yaw = -90.f, pitch = 0.f;
        float speed = 5.f, sens = 0.2f;

        // constructor with vectors
        explicit Camera(glm::vec3 position = glm::vec3(0, 0, 0), glm::vec3 up = glm::vec3(0, 1, 0), float yaw = -90.f, float pitch = 0.f)
        : forward(glm::vec3(0.0f, 0.0f, -1.0f)), speed(5.f), sens(0.2f)
        {
            this->position = position;
            this->worldUp = up;
            this->yaw = yaw;
            this->pitch = pitch;
            updateVectors();
        }

        // returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 getViewMatrix() const
        {
            return glm::lookAt(position, position + forward, up);
        }

        void processKeyInput(Direction direction, float deltaTime, bool onlyXZ = true)
        {
            auto moveDir = glm::vec3(0.f);
            auto localForward = onlyXZ ? glm::normalize(glm::vec3(forward.x, 0, forward.z)) : forward;
            auto localRight = onlyXZ ? glm::normalize(glm::vec3(right.x, 0, right.z)) : right;
            float moveSpeed = speed * deltaTime;
            if (direction == FORWARD)
                moveDir += localForward;
            if (direction == BACKWARD)
                moveDir -= localForward;
            if (direction == LEFT)
                moveDir -= localRight;
            if (direction == RIGHT)
                moveDir += localRight;
            if(moveDir != glm::vec3(0.f))
                position += glm::normalize(moveDir) * moveSpeed;
        }

        void processMouseInput(float xOffset, float yOffset)
        {
            xOffset *= sens;
            yOffset *= sens;

            yaw += xOffset;
            pitch += yOffset;

            // looking up/down constraints
            if(pitch > 89.f)
                pitch = 89.f;
            if(pitch < -89.f)
                pitch = -89.f;

            // update vectors
            updateVectors();
        }

    private:
        void updateVectors()
        {
            auto direction = glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                                       sin(glm::radians(pitch)),
                                       sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
            forward = glm::normalize(direction);
            right = glm::normalize(glm::cross(forward, worldUp));
            up = glm::normalize(glm::cross(right, forward));
        }
    };
#endif


#endif //ITU_GRAPHICS_PROGRAMMING_CAMERA_H
