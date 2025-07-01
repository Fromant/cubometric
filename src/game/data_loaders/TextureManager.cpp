#include "TextureManager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "../../Application.h"
#include "../../../3rdparty/stb_image.h"
#include "render/utils.h"

constexpr int LAYER_COUNT = 2048; //Opengl 4.5 provides at least that number of layers
constexpr int MIPMAP_LEVELS = 4;
constexpr int TEXTURE_WIDTH = 16;
constexpr int TEXTURE_HEIGHT = 16;

void TextureManager::Init(const std::string& folder_path) {
    glGenTextures(1, &textureArray);
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
        exit(EXIT_FAILURE);
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
        exit(EXIT_FAILURE);
    }

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, MIPMAP_LEVELS,GL_RGBA8,
                   TEXTURE_WIDTH, TEXTURE_HEIGHT, LAYER_COUNT);
    if (auto err = glGetError(); err != GL_NO_ERROR) {
        std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
        exit(EXIT_FAILURE);
    }

    stbi_set_flip_vertically_on_load(1);

    int layer = 0;

    for (const auto& file : std::filesystem::recursive_directory_iterator(folder_path)) {
        if (file.is_directory()) continue;
        std::string filename = file.path().string();
        std::replace(filename.begin(), filename.end(), '\\', '/');
        if (file.path().extension() != ".png" && file.path().extension() != ".jpg") {
            std::cerr << "Texture file extension " << file.path().extension().generic_string() << " not supported";
            exit(EXIT_FAILURE);
        }

        int w, h, channels;
        unsigned char* data = stbi_load(filename.c_str(), &w, &h, &channels, STBI_rgb_alpha);
        if (!data) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            std::cerr << stbi_failure_reason() << std::endl;
            exit(EXIT_FAILURE);
        }

        if (w != TEXTURE_WIDTH || h != TEXTURE_HEIGHT || channels != 4) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            std::cerr << "Invalid size:" << std::endl;
            std::cerr << "Got: " << w << 'x' << h << std::endl;
            std::cerr << "Expected size:" << TEXTURE_WIDTH << 'x' << TEXTURE_HEIGHT << std::endl;
            exit(EXIT_FAILURE);
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

        if (auto err = glGetError(); err != GL_NO_ERROR) {
            std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
            exit(EXIT_FAILURE);
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, TEXTURE_WIDTH, TEXTURE_HEIGHT, 1,
                        GL_RGBA,GL_UNSIGNED_BYTE, data);

        if (auto err = glGetError(); err != GL_NO_ERROR) {
            std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
            exit(EXIT_FAILURE);
        }

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        if (auto err = glGetError(); err != GL_NO_ERROR) {
            std::cerr << "Error during loading texture in line " << __LINE__ << ": " << err << std::endl;
            exit(EXIT_FAILURE);
        }
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        if (auto err = glGetError(); err != GL_NO_ERROR) {
            std::cerr << "Error generating mipmaps: " << err << std::endl;
            exit(EXIT_FAILURE);
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        stbi_image_free(data);

        textureMap.emplace(filename, layer++);
    }

    stbi_set_flip_vertically_on_load(0);
}

GLuint TextureManager::getTextureLayer(const std::string& name) const {
    auto it = textureMap.find(name);
    if (it == textureMap.end()) {
        std::cerr << "Texture " << name << " is not loaded!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return it->second;
}

GLuint TextureManager::loadTexture2D(const std::string& name, GLuint internalFormat, GLuint type, GLuint minFilter, GLuint magFilter) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    additionalTextures.emplace_back(textureID);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(name.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D,
                     0, internalFormat, width, height, 0, internalFormat, type, data);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Texture failed to load at path: " << name << std::endl;
        stbi_image_free(data);
        exit(EXIT_FAILURE);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    return textureID;
}


TextureManager::~TextureManager() {
    glDeleteTextures(additionalTextures.size(), additionalTextures.data());
    glDeleteTextures(1, &textureArray);
}
