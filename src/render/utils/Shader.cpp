#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <glad/glad.h>

Shader::Shader(const char *vertPath, const char *fragPath) {
    const auto vertex = LoadFile(vertPath);
    const auto fragment = LoadFile(fragPath);

    const auto vertex_c = vertex.c_str();
    const auto fragment_c = fragment.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_c, nullptr);
    glCompileShader(vertexShader);


    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint logSize = 0;

        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logSize);

        std::vector<GLchar> message = std::vector<GLchar>(logSize);

        glGetShaderInfoLog(vertexShader, logSize, nullptr, &message[0]);
        glDeleteShader(vertexShader);

        std::cout << std::string(message.begin(), message.end());
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment_c, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint logSize = 0;

        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logSize);

        std::vector<GLchar> message = std::vector<GLchar>(logSize);

        glGetShaderInfoLog(fragmentShader, logSize, nullptr, &message[0]);
        glDeleteShader(fragmentShader);

        std::cout << std::string(message.begin(), message.end());
    }


    // Link shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    // Cleanup shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() const {
    glUseProgram(ID);
}

std::string Shader::LoadFile(const char *path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Could not open file " + std::string(path));

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

