#include "GraphRenderer.h"

#include <glad/glad.h>
#include <algorithm>
#include <vector>
#include <glm/ext/matrix_clip_space.hpp>

#include "render/globals.h"

namespace debug {
    GraphRenderer::GraphRenderer(const Shader &shader, float x, float y, float width, float height)
        : shader(shader), xPos(x), yPos(y), width(width), height(height) {
        // Generate and bind VAO and VBO
        VAO = -1;
        VBO = -1;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *) 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    GraphRenderer::~GraphRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void GraphRenderer::UpdateData(const std::vector<float> &data) {
        if (data.empty()) return;

        // Find the maximum value for normalization
        float maxVal = std::max(*std::max_element(data.begin(), data.end()), 16.0f);

        vertices.clear();

        float spacing = width / data.size(); // Adjust spacing based on width

        for (size_t i = 0; i < data.size(); ++i) {
            float x = xPos + i * spacing; // Adjust x position based on graph's position
            float y_top = (data[i] / maxVal) * height; // Normalize to [0..height]

            vertices.emplace_back(x, yPos - height); // Bottom vertex
            vertices.emplace_back(x, yPos - height + y_top); // Top vertex
        }

        // Upload vertex data to GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void GraphRenderer::Render() const {
        const glm::mat4 projection = glm::ortho(-1.f, 1.f, -1.f, 1.f);
        shader.use();

        // Set the projection matrix uniform
        shader.setMat4("uProjection", projection);

        // Set color (e.g., green)
        shader.setVec3("uColor", glm::vec3(1, 1, 1));

        glBindVertexArray(VAO);

        // Draw all lines at once:
        glLineWidth(render::screenWidth * (vertices[2].x - vertices[0].x) / 2);
        glDrawArrays(GL_LINES, 0, vertices.size());

        glBindVertexArray(0);
    }
} // debug
