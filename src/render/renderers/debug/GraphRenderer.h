#ifndef GRAPHRENDERER_H
#define GRAPHRENDERER_H

#include <glm/glm.hpp>
#include <vector>
#include "../../utils/Shader.h"

namespace debug {
    class GraphRenderer {
    public:
        GraphRenderer(const Shader &shader, float x, float y, float width, float height);

        ~GraphRenderer();

        void UpdateData(const std::vector<float> &data);

        void Render() const;

    private:
        const Shader &shader;
        GLuint VAO, VBO;
        std::vector<glm::vec2> vertices;
        float xPos, yPos; // Position of the graph
        float width, height; // Size of the graph
    };
} // debug

#endif //GRAPHRENDERER_H
