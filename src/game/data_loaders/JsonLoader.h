#ifndef JSONSTREAM_H
#define JSONSTREAM_H
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class JsonLoader {
    std::string path;
    std::map<std::string, json> jsons;
public:
    explicit JsonLoader(const std::string &dir);
};


#endif //JSONSTREAM_H
