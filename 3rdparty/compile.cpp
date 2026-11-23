// TLS (thread_local) is broken in the w64devkit MinGW static build; stb's
// per-thread flip flag is unused here, so opt out of thread locals entirely.
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
