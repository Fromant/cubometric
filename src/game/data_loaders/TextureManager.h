#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>

class TextureManager {
    GLuint textureArray = 0;

    std::map<std::string, GLuint> textureMap;
    std::vector<GLuint> additionalTextures;

public:
    void Init(const std::string& folder_path);

    GLuint getTextureLayer(const std::string& name) const;

    GLuint getTextureArray() const { return textureArray; };

    GLuint loadTexture2D(const std::string& name,
        GLuint internalFormat = GL_RGB, GLuint type = GL_UNSIGNED_BYTE,
        GLuint minFilter = GL_NEAREST_MIPMAP_NEAREST, GLuint magFilter = GL_NEAREST);

    ~TextureManager();
};


#endif //TEXTUREMANAGER_H
