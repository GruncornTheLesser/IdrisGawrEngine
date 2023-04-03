#pragma once
#include <stdint.h>
#include <vector>
#include "Config.h"
namespace Gawr::Core {
	enum class DisplayMode { Fullscreen, Borderless, Windowed };

	struct DisplaySettings {
		DisplaySettings(const char* fp) : filepath("Config/Display.cfg") {
			auto configData = readConfig(filepath);

			appName = configData.at("appName");

			device = std::stoi(configData.at("device"));

			auto res = configData.at("resolution");
			resolution[0] = std::stoi(res.substr(0, res.find(' ')));
			resolution[1] = std::stoi(res.substr(res.find(' ') + 1));

			const std::vector<const char*> modes = { "Fullscreen", "Borderless", "Windowed" };
			uint32_t displayModeIndex = std::find(modes.begin(), modes.end(), configData.at("displayMode")) - modes.begin();
			dispayMode = static_cast<DisplayMode>(displayModeIndex);

			resizable = std::stoi(configData.at("resizable"));

			vsync = std::stoi(configData.at("vsync"));
		}

		void save() const {
			writeConfig(filepath, std::map<std::string, std::string>{
				std::pair("appName", appName),
				std::pair("device", std::to_string(device)),
				std::pair("resolution", std::to_string(resolution[0]) + " " + std::to_string(resolution[1])),
				std::pair("resizable", resizable ? "1" : "0"),
				std::pair("resizable", vsync ? "1" : "0"),
			});
		}
	
	public:
		std::string filepath;
		std::string appName;
		uint32_t device;
		uint32_t resolution[2];
		DisplayMode dispayMode;   // borderless/fullscreen/windowed
		bool	 resizable;
		bool	 vsync;
		
	};

	struct GraphicsSettings {
		
		uint32_t textureQuality;	// low/mid/high
		uint32_t meshQuality;		// low/mid/high
		uint32_t shadowQuality;		// low/mid/high
		uint32_t antiAliasing;		// FXAA/MSAAx2/MSAAx4/MSAAx8/TXAA
		bool ambientOcclusion;
		bool HDR;
		bool bloom;

		static GraphicsSettings load(const char* filepath) {
			auto configData = readConfig(filepath);

			GraphicsSettings settings;
			settings.textureQuality = std::stoi(configData.at("textureQuality"));
			settings.meshQuality = std::stoi(configData.at("meshQuality"));
			settings.shadowQuality = std::stoi(configData.at("shadowQuality"));
			settings.antiAliasing = std::stoi(configData.at("antiAliasing"));
			settings.ambientOcclusion = std::stoi(configData.at("ambientOcclusion"));
			settings.HDR = std::stoi(configData.at("HDR"));
			settings.bloom = std::stoi(configData.at("bloom"));
			
			return settings;
		}

		void save(const char* filepath) {
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
}