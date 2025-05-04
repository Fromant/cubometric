#include "TextureManager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "../../Application.h"
#include "../../../3rdparty/stb_image.h"

void TextureManager::Init(const std::string &folder_path) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    atlases.emplace_back(texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (glGetError()!=GL_NO_ERROR) {
        std::cerr << "Error during loading texture" << std::endl;
        exit(EXIT_FAILURE);
    }
    int x = 0;
    int y = 0;

    for (const auto &file: std::filesystem::recursive_directory_iterator(folder_path)) {
        if (file.is_directory()) continue;
        std::string filename = file.path().string();
        std::replace(filename.begin(), filename.end(), '\\', '/');
        if (file.path().extension() != ".png" && file.path().extension() != ".jpg") {
            std::cerr << "Texture File extension " << file.path().extension().generic_string() << " not supported";
            exit(EXIT_FAILURE);
        }

        int w, h, channels;
        unsigned char *data = stbi_load(filename.c_str(), &w, &h, &channels, STBI_rgb_alpha);
        if (!data) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            std::cerr << stbi_failure_reason() << std::endl;
            exit(EXIT_FAILURE);
        }

        if (w != TEX_SIZE || h != TEX_SIZE) {
            std::cerr << "Wrong texture size: " << w << "x" << h << std::endl;
            std::cerr << "At file : " << filename << std::endl;
            exit(EXIT_FAILURE);
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);
        if (glGetError()!=GL_NO_ERROR) {
            std::cerr << "Error during loading texture" << std::endl;
            exit(EXIT_FAILURE);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        // glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
        if (auto err = glGetError(); err !=GL_NO_ERROR) {
            std::cerr << "Error during loading texture: " << err << std::endl;
            exit(EXIT_FAILURE);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        if (auto err = glGetError(); err !=GL_NO_ERROR) {
            std::cerr << "Error during loading texture: " << err << std::endl;
            exit(EXIT_FAILURE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        const float uMin = static_cast<float>(x) / ATLAS_SIZE;
        const float vMin = static_cast<float>(y) / ATLAS_SIZE;

        textures.emplace(filename, TextureInfo{texture_id, uMin, vMin});

        x += TEX_SIZE;
        y += TEX_SIZE;

        if (x >= ATLAS_SIZE || y >= ATLAS_SIZE) {
            //gen new atlas
            glGenTextures(1, &texture_id);
            atlases.emplace_back(texture_id);
            x = 0;
            y = 0;
        }
    }
}

const TextureManager::TextureInfo &TextureManager::getTextureInfo(const std::string &filename) {
    return textures.at(filename);
}

TextureManager::~TextureManager() {
    glDeleteTextures(atlases.size(), atlases.data());
}
