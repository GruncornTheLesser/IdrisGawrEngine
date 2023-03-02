#pragma once
#include <string>
#include <map>

auto readConfig(std::string filepath)->std::map<std::string, std::string>;

auto writeConfig(std::string filepath, std::map<std::string, std::string> value)->void;
