#include <SDL3/SDL.h>
#include <exception>

#include "app/application.h"
#include "core/check.h"
#include "core/log.h"

namespace {

// SDL lifecycle guard: initialized before any window exists, quit after the
// application (and its render thread) is fully torn down.
struct SdlGuard {
    SdlGuard () { IH_CHECK (SDL_Init (SDL_INIT_VIDEO)); }
    ~SdlGuard () { SDL_Quit (); }
};

}  // namespace

int main (int /*argc*/, char** /*argv*/) {
    SdlGuard sdl;
    try {
        ih::app::Application app (1280, 720);
        return app.run ();
    } catch (const std::exception& e) {
        ih::core::logError (std::string ("Fatal: ") + e.what ());
        return 1;
    }
}
