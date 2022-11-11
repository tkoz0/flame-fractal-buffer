#pragma once

#include <iostream>

#include "../nlohmann/json.hpp"

class Json;

typedef std::vector<Json> JsonArray;
typedef std::unordered_map<std::string,Json> JsonObject;

// interface for reading data from nlohmann json
class Json: private nlohmann::json
{
public:
    // create json object from input stream or string
    Json(std::istream& input);
    Json(const std::string& input);
    Json(const nlohmann::json& input);
    // get value from json, returns true if set, false if type is wrong
    bool isNull() const;
    bool boolValue(bool& value) const;
    bool intValue(int64_t& value) const;
    bool floatValue(double& value) const;
    bool arrayValue(JsonArray& value) const;
    bool objectValue(JsonObject& value) const;
    // accesses list index, does not check type or bounds
    Json operator[](size_t index) const;
    // accesses object, does not check type or if key exists
    Json operator[](const std::string& key) const;
};
