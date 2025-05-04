#ifndef JSONSTREAM_H
#define JSONSTREAM_H
#include <string>
#include <unordered_map>

class JsonStream {
    std::unordered_map<std::string, std::string> data;

public:
    explicit JsonStream(const std::string &fileName);

    int getInt(const std::string &key);

    double getDouble(const std::string &key);

    const std::string &getString(const std::string &key);


    ~JsonStream();
};


#endif //JSONSTREAM_H
