#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include <glad/glad.h>

class TextureManager {
public:
    struct TextureInfo {
        GLuint atlasTexture;
        float uMin, vMin;
    };

private:
    std::unordered_map<std::string, TextureInfo> textures;
    std::vector<GLuint> atlases{};

public:
    static constexpr int TEX_SIZE = 16;
    static constexpr int ATLAS_SIZE = 16;
    // static constexpr float UV_STEP = static_cast<float>(TEX_SIZE) / ATLAS_SIZE;
    static constexpr float UV_STEP = 1;

    GLuint getLastAtlas() const {
        return atlases.back();
    }

    void Init(const std::string &folder_path);

    const TextureInfo &getTextureInfo(const std::string &name);

    ~TextureManager();
};


#endif //TEXTUREMANAGER_H
