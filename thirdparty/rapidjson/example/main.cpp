#include "thirdparty/rapidjson/rapidjson.h"
#include "thirdparty/rapidjson/document.h"
#include <iostream>

int main(int argc, char* argv[]) {

    const char* json = "{\"a\": \"1\", \"b\": 2}";

    rapidjson::Document d;
    d.Parse<0>(json);

    std::cout << d["a"].GetString() << std::endl;
    std::cout << d["b"].GetInt() << std::endl;
    return 0;
}
