#include <stdint.h>
#include "../Config.h"

struct DisplaySettings {
	const char* filepath;
	std::string appName;
	uint32_t device;
	uint32_t resolution[2];
	uint32_t dispayMode;   // borderless/fullscreen/windowed
	bool	 resizable;
	bool	 vsync;

	DisplaySettings(const char* fp) : filepath(fp) {
		auto configData = readConfig(filepath);
		appName = configData.at("appName");
		device = std::stoi(configData.at("device"));
		auto res = configData.at("device");
		resolution[0] = std::stoi(res.substr(0, res.find(' ')));
		resolution[1] = std::stoi(res.substr(res.find(' ') + 1));
		dispayMode = std::stoi(configData.at("displayMode"));
		resizable = std::stoi(configData.at("resizable"));
		vsync = std::stoi(configData.at("vsync"));
	}

	void save() {
		writeConfig(filepath, std::map<std::string, std::string>{
			std::pair("appName", appName),
			std::pair("device", std::to_string(device)),
			std::pair("resolution", std::to_string(resolution[0]) + " " + std::to_string(resolution[1])),
			std::pair("resizable", resizable ? "1" : "0"),
			std::pair("resizable", vsync ? "1" : "0"),
		});
	}
};


struct GraphicsSettings {
	const char* filepath;
	uint32_t textureQuality;	// low/mid/high
	uint32_t meshQuality;		// low/mid/high
	uint32_t shadowQuality;		// low/mid/high
	uint32_t antiAliasing;		// FXAA/MSAAx2/MSAAx4/MSAAx8/TXAA
	bool ambientOcclusion;
	bool HDR;
	bool bloom;

	GraphicsSettings(const char* fp) : filepath(fp) {
		auto configData = readConfig(filepath);

		textureQuality = std::stoi(configData.at("textureQuality"));
		meshQuality = std::stoi(configData.at("meshQuality"));
		shadowQuality = std::stoi(configData.at("shadowQuality"));
		antiAliasing = std::stoi(configData.at("antiAliasing"));
		ambientOcclusion = std::stoi(configData.at("ambientOcclusion"));
		HDR = std::stoi(configData.at("HDR"));
		bloom = std::stoi(configData.at("bloom"));
	}

	void save() {
		writeConfig(filepath, std::map<std::string, std::string>{
			std::pair("textureQuality", std::to_string(textureQuality)),
				std::pair("meshQuality", std::to_string(meshQuality)),
				std::pair("shadowQuality", std::to_string(shadowQuality)),
				std::pair("antiAliasing", antiAliasing ? "1" : "0"),
				std::pair("ambientOcclusion", ambientOcclusion ? "1" : "0"),
				std::pair("HDR", HDR ? "1" : "0"),
				std::pair("bloom", bloom ? "1" : "0"),
		});
	}
};
