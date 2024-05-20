#include "AddonRenderer.h"
//#include <imgui_impl_dx11.h>

using json = nlohmann::json;

/* Proto */
void centerText(std::string text, float yOffset);
void fade();

/* Render subfunctions */
void renderSectorInfo();
void renderDebugInfo();

std::thread animationThread;
bool animating = false;
bool fadingIn = false;
float opacity = 0.0f;

CurrentMapService currentMapService = CurrentMapService();
int currentSectorId = -1;

Renderer::Renderer() {}
Renderer::~Renderer() {
	if (animationThread.joinable()) {
		animationThread.join();
	}
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
	renderSectorInfo();
	renderDebugInfo();
}

void centerText(std::string text, float yOffset) {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;

	// Center Text voodoo
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	float textX = (windowSize.x - textSize.x) / 2.0f;
	float textY = windowSize.y / yOffset;

	ImGui::SetCursorScreenPos(ImVec2(textX + 2, textY + 2));
	ImGui::TextColored(ImVec4(0, 0, 0, opacity), text.c_str()); // shadow
	ImGui::SetCursorPos(ImVec2(textX, textY));
	ImGui::TextColored(ImVec4(255,255,255, opacity), text.c_str()); // text
}

/// <summary>
/// Original Author: Delta
/// Additions to fade out after sleep: Pirate
/// 
/// Routine to fade in/fade out content hooked on the opacity flag.
/// Music Tip: NOTHING MORE - FADE IN/FADE OUT:
/// https://www.youtube.com/watch?v=wBC3Tl0dg4M
/// </summary>
void fade() {
	// fade in
	while (true)
	{
		opacity += 0.05f;
		if (opacity > 1) { 
			opacity = 1.0f; 
			break;
		}

		Sleep(35);
	}
	Sleep(3000); // sleep first so the text stays a little
	// fade out
	while (true)
	{
		opacity -= 0.05f;

		if (opacity < 0.0f) { 
			opacity = 0.0f; 
			break;
		}

		Sleep(35);
	}
	animating = false;
}

void renderSectorInfo() {

	// TODO possible skips for render:
	// - when in map view
	// - when in competitive modes
	// - when idk lmao
	// TODO possible skips into settings?

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;

	// Make the next Window go over the entire screen for easier calculations
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	// And make sure to disable all interaction with it properly *cough*

	MapData* currentMap = currentMapService.getCurrentMap();
	if (currentMap == nullptr) return;

	if (currentSectorId != currentMap->currentSector.id) {
		currentSectorId = currentMap->currentSector.id;
		// we need to render! start the animation thread if not animating already
		if (!animating) {
			animationThread = std::thread(fade);
			animationThread.detach();
		}
	}

	if (ImGui::Begin("MapdetailsFrame", (bool*)0,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoMouseInputs))
	{
		// Default Font
		// Sector name in nice and big
		ImFont* font = (ImFont*)NexusLink->FontBig;
		float originalFontScale = font->Scale;
		font->Scale = 1.5f;

		ImGui::PushFont((ImFont*)NexusLink->FontBig);
		centerText(currentMap->currentSector.name, 3.5f);
		font->Scale = originalFontScale;
		ImGui::PopFont();

		// Map + Region + Continent (maybe we can figure this out in settings what user wants?)
		ImGui::PushFont((ImFont*)NexusLink->Font);
		std::string miniText = "";
		miniText.append(currentMap->continentName);
		miniText.append(" | ");
		miniText.append(currentMap->regionName);
		miniText.append(" | ");
		miniText.append(currentMap->name);

		centerText(miniText, 3.8f);
		ImGui::PopFont();

		/*
		* Once we figured out custom fonts, use the right font for our character race, or any other according to settings (?)
		*
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
		*/
	}
	ImGui::End();
}

void renderDebugInfo() {
	if (!showDebug) return;

	if(ImGui::Begin("TyrianRegionsDebug"))
	{
		ImGui::PushFont((ImFont*)NexusLink->Font);

		ImGui::Text("Player information");
		ImGui::Text(("GlobalX: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.X)).c_str());
		ImGui::Text(("GlobalY: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.X)).c_str());

		ImGui::Separator();
		ImGui::Text("Mumble Information");
		ImGui::Text(("Map Id: " + std::to_string(MumbleLink->Context.MapID)).c_str());

		ImGui::Separator();
		ImGui::Text("Current Map Data");
		MapData* currentMap = currentMapService.getCurrentMap();
		if (currentMap == nullptr) {
			ImGui::TextColored(ImVec4(255, 0, 0, 1), "No map found in inventory!");
		}
		else {
			ImGui::Text(("Map Id: " + std::to_string(currentMap->id)).c_str());
			ImGui::Text(("Map Name: " + currentMap->name).c_str());
			ImGui::Text(("Region Id: " + std::to_string(currentMap->regionId)).c_str());
			ImGui::Text(("Region Name: " + currentMap->regionName).c_str());
			ImGui::Text(("Continent Id: " + std::to_string(currentMap->continentId)).c_str());
			ImGui::Text(("Continent Name: " + currentMap->continentName).c_str());
			ImGui::Text(("Current Sector Id: " + std::to_string(currentMap->currentSector.id)).c_str());
			ImGui::Text(("Current Sector Name: " + currentMap->currentSector.name).c_str());
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Map Inventory Data", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (currentMap == nullptr) {
				ImGui::TextColored(ImVec4(255, 0, 0, 1), "No map found in inventory!");
			}
			else {
				gw2::map* inventoryMap = mapInventory->getMapInfo("en", currentMap->id);
				ImGui::Text(("Map Id: " + std::to_string(inventoryMap->id)).c_str());
				ImGui::Text(("Map Name: " + inventoryMap->name).c_str());
				ImGui::Text(("Region Id: " + std::to_string(inventoryMap->regionId)).c_str());
				ImGui::Text(("Region Name: " + inventoryMap->regionName).c_str());
				ImGui::Text(("Continent Id: " + std::to_string(inventoryMap->continentId)).c_str());
				ImGui::Text(("Continent Name: " + inventoryMap->continentName).c_str());
				ImGui::Text(("MinLevel: " + std::to_string(inventoryMap->minLevel)).c_str());
				ImGui::Text(("MaxLevel: " + std::to_string(inventoryMap->maxLevel)).c_str());
				if (ImGui::CollapsingHeader("Sectors")) {
					for (auto sector : inventoryMap->sectors) {
						if (ImGui::CollapsingHeader((std::to_string(sector.second.id) + ": " + sector.second.name).c_str())) {
							json j = sector.second;
							ImGui::Text(j.dump(4).c_str());
						}
					}
				}
			}
		}

		ImGui::Separator();

		ImGui::PopFont();
	}
	ImGui::End();
}