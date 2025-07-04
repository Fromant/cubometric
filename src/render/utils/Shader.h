#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <glad/glad.h>

class Shader {
public:
    Shader(const char* vertPath, const char* fragPath, const char* geomPath = nullptr);

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string& name, const glm::mat3& mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setFloat(const std::string& name, float val) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), val);
    }

    void setSampler(const std::string& name, GLuint sampler) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), sampler);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setIVec3(const std::string& name, const glm::ivec3 value) const {
        glUniform3i(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
    }

    void setInt32(const std::string& name, const int32_t value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setInt32s(const std::string& name, const std::vector<int32_t>& values) const {
        glUniform1iv(glGetUniformLocation(ID, name.c_str()), values.size(), values.data());
    }

    GLuint getID() const { return ID; }

    ~Shader();

private:
    GLuint ID;

    static std::string LoadFile(const char* path);
};
