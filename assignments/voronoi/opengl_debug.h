#ifndef OPENGL_DEBUG_H
#define OPENGL_DEBUG_H

#include <glad/glad.h>
#include <cassert>

/**
* \brief Wraps an OpenGL call in error handling, for debugging.
*/
#define GLCall(x) GLClearError();\
	x;\
	assert(GLLogCall(#x, __FILE__, __LINE__))

/**
 * \brief Clears the OpenGL error stack.
 */
static void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

/**
 * \brief A lookup table for converting error codes to readable names.
 * Remember to subtract the base value (0x500) to get to the correct index.
 */
static const char *errorCodeToErrorNameLookupTable[] = {
        "GL_INVALID_ENUM",
        "GL_INVALID_VALUE",
        "GL_INVALID_OPERATION",
        "GL_STACK_OVERFLOW",
        "GL_STACK_UNDERFLOW",
        "GL_OUT_OF_MEMORY",
        "GL_INVALID_FRAMEBUFFER_OPERATION",
        "GL_CONTEXT_LOST"
};

/**
* \brief Reports all OpenGL errors.
* \param function The name of the function that is being tested
* \param file The file containing the tested function
* \param line The line number of the error
* \return True if there were no errors, false otherwise
*/
static bool GLLogCall(const char* function, const char* file, int line)
{
    bool hadErrors = false;
    while (GLenum error = glGetError())
    {
        hadErrors = true;
        std::cout << "[OpenGL Error] (";
        if (error <= 0x507)
        {
            const char *errorName = errorCodeToErrorNameLookupTable[error - 0x0500];
            std::cout << errorName;
        }
        else
        {
            std::cout << error;
        }
        std::cout << "):" << function << " " << file << ":" << line << std::endl;

    }
    return !hadErrors;
}

#endif