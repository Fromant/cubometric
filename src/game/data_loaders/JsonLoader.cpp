#include "JsonLoader.h"

#include <fstream>
#include <iostream>

JsonLoader::JsonLoader(const std::string& dir) {
    for (const auto entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;

        jsons.emplace(entry.path().string(), json::parse(std::ifstream(entry.path())));

        std::cout << entry.path() << ' ' << jsons[entry.path().string()] << std::endl;

    }
}
