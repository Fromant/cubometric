#ifndef DEBUGRENDERER_H
#define DEBUGRENDERER_H

#include "GraphRenderer.h"

namespace debug {
    class DebugRenderer {
        bool show = false;

        std::vector<float> frametimes = std::vector<float>(60, 0);

        const Shader graphShader{"../shaders/debug/vert.glsl","../shaders/debug/frag.glsl"};

        GraphRenderer graphRenderer{graphShader, -1,1,0.15, 0.1};

    public:
        void setEnabled() { show = true; }
        void setDisabled() { show = false; }

        void switchEnabled() { show = !show; }

        void Render(float frametime, int windowWidth, int windowHeight) {
            if (show) {
                frametimes.erase(frametimes.begin());
                frametimes.push_back(frametime);

                graphRenderer.UpdateData(frametimes);
                graphRenderer.Render();
            }
        }
    };
} // debug

#endif //DEBUGRENDERER_H
