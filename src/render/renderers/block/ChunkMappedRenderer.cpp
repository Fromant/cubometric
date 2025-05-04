#include "ChunkMappedRenderer.h"

#include "../../buffers/GPUBuffer.h"
#include "../../../../3rdparty/stb_image.h"

void ChunkMappedRenderer::Init() {
    shader = new Shader("../shaders/face/vert.glsl", "../shaders/face/frag.glsl");

    GLuint tempVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &tempVBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, tempVBO); // Must bind a VBO!

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));

    glBindVertexArray(0);

    // Cleanup temporary VBO
    glDeleteBuffers(1, &tempVBO);

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

void ChunkMappedRenderer::Render(const glm::mat4 &view, const glm::mat4 &proj, GPUBuffer *buffer,
                                 int numVertices) {
    if (!buffer || numVertices == 0) return;

    shader->use();
    glBindVertexArray(VAO);
    buffer->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader->setSampler("atlasTexture", 0);
    shader->setMat4("view", view);
    shader->setMat4("projection", proj);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    // buffer->notifySubmitted();
}
