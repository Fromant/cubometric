#pragma once

#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>

#ifdef DEBUG
#define GL_CHECK(stmt) do { \
stmt; \
CheckOpenGLError(#stmt, __FILE__, __LINE__); \
} while (0)
#else
#define GL_CHECK(stmt) stmt
#endif
