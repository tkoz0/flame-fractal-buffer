#include "json_small.hpp"

// parse JSON from a source, ignoring comments
template <typename T>
nlohmann::json parse_json(T&& input)
{
    return nlohmann::json::parse(input,nullptr,true,true);
}

Json::Json(std::istream& input): json(parse_json(input))
{
}

Json::Json(const std::string& input): json(parse_json(input))
{
}

Json::Json(const nlohmann::json& input): json(input)
{
}

bool Json::isNull() const
{
    return json.is_null();
}

bool Json::boolValue(bool& value) const
{
    if (!json.is_boolean())
        return false;
    value = json.get<bool>();
    return true;
}

bool Json::intValue(int64_t& value) const
{
    if (!json.is_number_integer())
        return false;
    value = json.get<int64_t>();
    return true;
}

bool Json::floatValue(double& value) const
{
    if (!json.is_number_float())
        return false;
    value = json.get<double>();
    return true;
}

bool Json::arrayValue(JsonArray& value) const
{
    if (!json.is_array())
        return false;
    auto tmp = json.get<std::vector<nlohmann::json>>();
    value.clear();
    value.reserve(tmp.size());
    for (size_t i = 0; i < tmp.size(); ++i)
        value.push_back(Json(tmp[i]));
    return true;
}

bool Json::objectValue(JsonObject& value) const
{
    if (!json.is_object())
        return false;
    auto tmp = json.get<std::unordered_map<std::string,nlohmann::json>>();
    value.clear();
    value.reserve(tmp.size());
    for (auto iter = tmp.begin(); iter != tmp.end(); ++iter)
        value.insert(*iter);
    return true;
}

Json Json::operator[](size_t index) const
{
    return Json(json.at(index));
}

Json Json::operator[](const std::string& key) const
{
    return Json(json.at(key));
}
