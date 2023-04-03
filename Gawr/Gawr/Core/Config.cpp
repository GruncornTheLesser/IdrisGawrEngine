#pragma once
#include "config.h"
#include <fstream>
#include <vector>

std::map<std::string, std::string> readConfig(std::string filepath)
{

	std::ifstream iconfig{ filepath };
	std::map<std::string, std::string> result;

	std::string line;
	while (std::getline(iconfig, line))
	{
		// find assignment delimiter
		size_t pos = line.find('=');

		// if no assignment delimiter exists skip
		if (pos == -1) continue;

		// get key
		size_t keybegin = line.find_first_not_of(" ", 0);
		size_t keycount = line.find_last_not_of(" ", pos - 1) + 1 - keybegin;
		std::string key = line.substr(keybegin, keycount);

		// get value
		size_t valbegin = line.find_first_not_of(" ", pos + 1);
		size_t valcount = line.find_last_not_of(" ") + 1 - valbegin;
		std::string value = line.substr(valbegin, valcount);

		result.emplace(key, value);
	}

	iconfig.close();

	return result;
}

void writeConfig(std::string filepath, std::map<std::string, std::string> value) {
	std::ifstream iconfig(filepath);
	std::ofstream oconfig(filepath + ".temp");

	if (!iconfig.is_open())
		throw std::exception("cannot open config file");

	if (!oconfig.is_open())
		throw std::exception("cannot create temporary file");

	std::string line;
	while (std::getline(iconfig, line))
	{
		// find assignment delimiter

		// if no assignment delimiter exists skip
		size_t pos;
		std::string key;

		[&]()->void
		{
			pos = line.find('=');
			if (pos == std::string::npos) return;

			// get key
			size_t keybegin = line.find_first_not_of(" ", 0);
			size_t keycount = line.find_last_not_of(" ", pos - 1) - keybegin + 1;
			key = line.substr(keybegin, keycount);

			auto it = value.find(key);
			if (it == value.end()) return;

			line.replace(pos + 1, it->second.size(), it->second.c_str());
			line.erase(pos + 1 + it->second.size(), it->second.size() - (pos + 1 + it->second.size()));
		}();

		oconfig << line << '\n';
	}
	oconfig.flush();
	iconfig.close();
	oconfig.close();

	std::remove((filepath).c_str());
	std::rename((filepath + ".temp").c_str(), filepath.c_str());
}
