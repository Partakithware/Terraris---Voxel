// include/Shader.h

#ifndef SHADER_H
#define SHADER_H

// Core OpenGL/GLAD includes
#include <glad/glad.h>
// File and string handling
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
// GLM for matrix/math utilities
#include <glm/glm.hpp>

class Shader {
public:
    // The program ID (handle) given by OpenGL after linking the shaders
    unsigned int ID; 

    /**
     * @brief Constructor that reads and builds the shader program from files.
     * * @param vertexPath Path to the vertex shader source file (e.g., "shaders/basic.vs").
     * @param fragmentPath Path to the fragment shader source file (e.g., "shaders/basic.fs").
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Activates the shader program for use in rendering.
     */
    void use();

    // --- Utility functions for setting uniforms (data passed to the GPU) ---

    // Set a boolean uniform value
    void setBool(const std::string &name, bool value) const;
    // Set an integer uniform value
    void setInt(const std::string &name, int value) const;
    // Set a floating-point uniform value
    void setFloat(const std::string &name, float value) const;
    // Set a 4x4 matrix uniform value (crucial for model, view, and projection matrices)
    void setMat4(const std::string &name, const glm::mat4 &mat) const; 

    // Set a 3-component vector uniform value
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;

private:
    /**
     * @brief Utility function for checking shader compilation/linking errors.
     * * @param shader The ID of the shader object or program.
     * @param type The type ("VERTEX", "FRAGMENT", or "PROGRAM") for logging.
     */
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif
