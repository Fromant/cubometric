#include "BlockFaceRenderer.h"

#include <iostream>
#include <ostream>

#include "../../../../3rdparty/stb_image.h"
#include "../../../game/data_loaders/TextureManager.h"

namespace render {

    void BlockFaceRenderer::Init() {
        shader = new Shader("../shaders/face/vert.glsl", "../shaders/face/frag.glsl");
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        //only 1 texture lol
        glGenTextures(1, &texture);
        int w, h, channels;
        stbi_set_flip_vertically_on_load(true);
        auto image = stbi_load("../assets/textures/blocks/dirt.png", &w, &h, &channels, STBI_rgb_alpha);
        glBindTexture(GL_TEXTURE_2D, texture);
        if (channels != 4) {
            std::cerr << "channels != 4" << std::endl;
            exit(EXIT_FAILURE);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);

        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void BlockFaceRenderer::updateVertices(const std::vector<FaceInstance> &faces) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, faces.size() * sizeof(FaceInstance), faces.data(), GL_DYNAMIC_DRAW);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);
        numVertices = faces.size() * 6; //six vertices per face
        // glBindVertexArray(0);
    }

    void BlockFaceRenderer::Render(const glm::mat4 &view, const glm::mat4 &proj) const {
        shader->use();
        glBindVertexArray(vao);
        shader->setMat4("view", view);
        shader->setMat4("projection", proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader->setSampler("atlasTexture", 0);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
    }


    BlockFaceRenderer::~BlockFaceRenderer() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &texture);
    }
} // render
