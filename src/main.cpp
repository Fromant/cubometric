#include "Application.h"
#include "game/data_loaders/JsonLoader.h"

int main() {
    // JsonLoader k("assets");
    Application a{};
    a.Run();
    return 0;
}
