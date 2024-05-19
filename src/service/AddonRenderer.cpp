#include "AddonRenderer.h"
//#include <imgui_impl_dx11.h>

Renderer::Renderer() {
	this->currentMapService = CurrentMapService();
}
Renderer::~Renderer() {
	//this->currentMapService.~CurrentMapService();
}

void Renderer::preRender(ImGuiIO& io) {
	if (!fontsLoaded) {
		// Load fonts into a custom atlas
		fonts = std::map<std::string, ImFont*>();

		//ImFontAtlas nexusAtlas = *io.Fonts;
		// initialize with nexusAtlas as base, hopefully
		newFontAtlas = new ImFontAtlas(); // nexusAtlas);

		std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
		fonts.emplace("fontCharr", loadFont(io, pathFolder + "/font_charr.ttf", 48.0f));
		fonts.emplace("fontHuman", loadFont(io, pathFolder + "/font_human.ttf", 48.0f));
		fonts.emplace("fontSylvari", loadFont(io, pathFolder + "/font_sylvari.ttf", 48.0f));
		fonts.emplace("fontNorn", loadFont(io, pathFolder + "/font_norn.ttf", 48.0f));
		fonts.emplace("fontAsura", loadFont(io, pathFolder + "/font_asura.ttf", 48.0f));

		newFontAtlas->Build();
		fontsLoaded = true;
	}

	defaultFontAtlas = io.Fonts;
	io.Fonts = newFontAtlas;

	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_CreateDeviceObjects();
}

void Renderer::postRender(ImGuiIO& io) {
	if (defaultFontAtlas != nullptr) {
		io.Fonts = defaultFontAtlas;

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}
}

ImFont* Renderer::loadFont(ImGuiIO& io, std::string path, float size) {
	//ImFont* newFont = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
	ImFont* newFont = newFontAtlas->AddFontFromFileTTF(path.c_str(), size);
	if (newFont == nullptr) {
		// TODO log error
		return nullptr;
	}
	return newFont;
}

void Renderer::render() {
	if (ImGui::Begin("MapdetailsFrame", (bool*)0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
	{

		ImGui::PushFont((ImFont*)NexusLink->Font);
		if (ImGui::GetFont() == nullptr) {
			APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "NexusLink->Font = nullptr");
		}
		ImGui::Text("Map Details Frame");
		ImGui::PopFont();

		MapData* currentMap = currentMapService.getCurrentMap();

		if (currentMap != nullptr) {
			// Default Font
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Default font = nullptr");
			}
			ImGui::Text(std::to_string(currentMap->id).c_str());
			ImGui::Text(currentMap->continentName.c_str());
			ImGui::Text(currentMap->name.c_str());
			ImGui::Text(currentMap->currentSector.name.c_str());

			// Charr specific Font
			ImGui::PushFont(fonts["fontCharr"]);
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "fontCharr = nullptr");
			}

			ImGui::Text(("Current Map: " + currentMap->name).c_str());
			ImGui::Text(("Current Sector: " + currentMap->currentSector.name).c_str());
			ImGui::PopFont();

			// Human specific Font
			ImGui::PushFont(fonts["fontHuman"]);
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "fontHuman = nullptr");
			}

			ImGui::Text(("Current Map: " + currentMap->name).c_str());
			ImGui::Text(("Current Sector: " + currentMap->currentSector.name).c_str());
			ImGui::PopFont();

			// Sylvari specific Font
			ImGui::PushFont(fonts["fontSylvari"]);
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "fontSylvari = nullptr");
			}

			ImGui::Text(("Current Map: " + currentMap->name).c_str());
			ImGui::Text(("Current Sector: " + currentMap->currentSector.name).c_str());
			ImGui::PopFont();

			// Norn specific Font
			ImGui::PushFont(fonts["fontNorn"]);
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "fontNorn = nullptr");
			}

			ImGui::Text(("Current Map: " + currentMap->name).c_str());
			ImGui::Text(("Current Sector: " + currentMap->currentSector.name).c_str());
			ImGui::PopFont();

			// Asura specific Font
			ImGui::PushFont(fonts["fontAsura"]); 
			if (ImGui::GetFont() == nullptr) {
				APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "fontAsura = nullptr");
			}

			ImGui::Text(("Current Map: " + currentMap->name).c_str());
			ImGui::Text(("Current Sector: " + currentMap->currentSector.name).c_str());
			ImGui::PopFont();
		}
		else {
			ImGui::Text("No current map detected. :(");
		}

	}
	ImGui::End();
}