#include "json_small.hpp"

Json::Json()
{
}

Json::Json(std::istream& input):
        nlohmann::json(nlohmann::json::parse(input,nullptr,true,true))
{
}

Json::Json(const std::string& input):
        nlohmann::json(nlohmann::json::parse(input,nullptr,true,true))
{
}

Json::Json(const nlohmann::json& input): nlohmann::json(input)
{
}

bool Json::isNull() const
{
    return this->is_null();
}

bool Json::boolValue(bool& value) const
{
    if (!this->is_boolean())
        return false;
    value = this->get<bool>();
    return true;
}

bool Json::intValue(int64_t& value) const
{
    if (!this->is_number_integer())
        return false;
    value = this->get<int64_t>();
    return true;
}

bool Json::floatValue(double& value) const
{
    if (!this->is_number_float())
        return false;
    value = this->get<double>();
    return true;
}

bool Json::stringValue(std::string& value) const
{
    if (!this->is_string())
        return false;
    value = this->get<std::string>();
    return true;
}

bool Json::arrayValue(JsonArray& value) const
{
    if (!this->is_array())
        return false;
    auto tmp = this->get<std::vector<nlohmann::json>>();
    value.clear();
    value.reserve(tmp.size());
    for (size_t i = 0; i < tmp.size(); ++i)
        value.push_back(Json(tmp[i]));
    return true;
}

bool Json::objectValue(JsonObject& value) const
{
    if (!this->is_object())
        return false;
    auto tmp = this->get<std::unordered_map<std::string,nlohmann::json>>();
    value.clear();
    value.reserve(tmp.size());
    for (auto iter = tmp.begin(); iter != tmp.end(); ++iter)
        value.insert(std::make_pair(iter->first,Json(iter->second)));
    return true;
}

bool Json::valueAt(size_t index, Json& value) const
{
    if (!this->is_array() || index >= this->size())
        return false;
    value = Json(this->at(index));
    return true;
}

bool Json::valueAt(const std::string& key, Json& value) const
{
    if (!this->is_object() || this->find(key) == this->end())
        return false;
    value = Json(this->at(key));
    return true;
}

bool Json::valueAt(const char *key, Json& value) const
{
    if (!this->is_object() || this->find(key) == this->end())
        return false;
    value = Json(this->at(key));
    return true;
}

Json Json::operator[](size_t index) const
{
    return Json(this->at(index));
}

Json Json::operator[](const std::string& key) const
{
    return Json(this->at(key));
}

Json Json::operator[](const char *key) const
{
    return Json(this->at(key));
}

Json& Json::operator=(const Json& a)
{
    static_cast<nlohmann::json&>(*this) = static_cast<const nlohmann::json&>(a);
    return *this;
}

bool Json::operator==(const Json& a)
{
    return static_cast<const nlohmann::json&>(*this) == static_cast<const nlohmann::json&>(a);
}

std::ostream& operator<<(std::ostream& os, const Json& json)
{
    os << static_cast<const nlohmann::json&>(json);
    return os;
}
