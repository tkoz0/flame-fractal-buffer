#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

#include "../nlohmann/json.hpp"

class Json;

typedef std::vector<Json> JsonArray;
typedef std::unordered_map<std::string,Json> JsonObject;

// interface for reading data from nlohmann json
class Json: private nlohmann::json
{
public:
    // create json object from input stream or string
    Json();
    Json(std::istream& input);
    Json(const std::string& input);
    Json(const nlohmann::json& input);
    // get value from json, returns true if set, false if type is wrong
    bool isNull() const;
    bool boolValue(bool& value) const;
    bool intValue(int64_t& value) const;
    bool floatValue(double& value) const;
    bool stringValue(std::string& value) const;
    bool arrayValue(JsonArray& value) const;
    bool objectValue(JsonObject& value) const;
    bool valueAt(size_t index, Json& value) const;
    bool valueAt(const std::string& key, Json& value) const;
    bool valueAt(const char *key, Json& value) const;
    // accesses list index, does not check type or bounds
    Json operator[](size_t index) const;
    // accesses object, does not check type or if key exists
    Json operator[](const std::string& key) const;
    Json operator[](const char *key) const;
    // other operators
    Json& operator=(const Json& a);
    bool operator==(const Json& a);
    // ostream operator
    friend std::ostream& operator<<(std::ostream& os, const Json& json);
};
