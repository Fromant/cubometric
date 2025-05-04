#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>

constexpr void CheckOpenGLError(const char *stmt, const char *fname, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}


#ifdef DEBUG
#define GL_CHECK(stmt) do { \
stmt; \
CheckOpenGLError(#stmt, __FILE__, __LINE__); \
} while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

#endif //UTILS_H
