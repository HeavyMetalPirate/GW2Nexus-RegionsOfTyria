#include "AddonRenderer.h"
#include <fstream>


using json = nlohmann::json;

/* Proto */
void centerText(std::string text, float yOffset);
void fade();

/* Render subfunctions */
void renderSectorInfo();
void renderDebugInfo();

std::thread animationThread;
bool animating = false;
bool cancelAnimation = false;
float opacity = 0.0f;

CurrentMapService currentMapService = CurrentMapService();
int currentSectorId = -1;

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::unload() {
	if (animationThread.joinable()) {
		animationThread.join();
	}
}

void Renderer::preRender(ImGuiIO& io) {
	/*
	// Reference as to why this should work as implemented:
	// https://github.com/ocornut/imgui/issues/2311#issuecomment-460039964
	// However it breaks everything Nexus has set up on its device.
	// Which is, suboptimal, to say the least.
	if (!fontsLoaded) {
		
		fonts = std::map<std::string, ImFont*>();

		std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
		fonts.emplace("fontCharr", loadFont(io, pathFolder + "/font_charr.ttf", 20.0f));
		fonts.emplace("fontHuman", loadFont(io, pathFolder + "/font_human.ttf", 20.0f));
		fonts.emplace("fontSylvari", loadFont(io, pathFolder + "/font_sylvari.ttf", 20.0f));
		fonts.emplace("fontNorn", loadFont(io, pathFolder + "/font_norn.ttf", 20.0f));
		fonts.emplace("fontAsura", loadFont(io, pathFolder + "/font_asura.ttf", 40.0f));
		io.Fonts->Build();
		ImGui_ImplDX11_InvalidateDeviceObjects();
		
		/*
		ImGuiContext* ctx = ((ImGuiContext*)APIDefs->ImguiContext);
		IDXGISwapChain* swapchain = ((IDXGISwapChain*)APIDefs->SwapChain);
		swapchain->GetDevice();


		io.Fonts->Build();
		ImGui_ImplDX11_InvalidateDeviceObjects();
		* /

		fontsLoaded = true;
	}
	*/
}

void Renderer::postRender(ImGuiIO& io) {
	/*if (defaultFontAtlas != nullptr) {
		io.Fonts = defaultFontAtlas;

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}*/
}

//ImFont* Renderer::loadFont(ImGuiIO& io, std::string path, float size) {
	/*ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;
	ImFont* newFont = io.Fonts->AddFontFromFileTTF(path.c_str(), size, &fontConfig);

	if (newFont == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("Could not load font: " + path).c_str());
		return nullptr;
	}
	io.Fonts->Build();
	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_CreateDeviceObjects();
	return newFont;*/
//}

//std::vector<unsigned char> Renderer::readFontFile(const char* filename) {
	/*std::ifstream file(filename, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<unsigned char> buffer(size);
	if (file.read((char*)buffer.data(), size))
	{
		return buffer;
	}
	else
	{
		// Handle error
		return std::vector<unsigned char>();
	}*/
//}

void Renderer::render() {
	if (unloading) return;
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
	cancelAnimation = false;
	try {
		// fade in
		while (true)
		{
			if (unloading || cancelAnimation) {
				opacity = 0.0f;
				return;
			}
			opacity += 0.05f;
			if (opacity > 1) {
				opacity = 1.0f;
				break;
			}

			Sleep(35);
		}
		for (int i = 0; i < 3000;i++) {
			if (unloading || cancelAnimation) {
				opacity = 0.0f;
				return;
			}
			Sleep(1); // sleep first so the text stays a little
		}
		// fade out
		while (true)
		{
			if (unloading || cancelAnimation) {
				opacity = 0.0f;
				return;
			}

			opacity -= 0.05f;

			if (opacity < 0.0f) {
				opacity = 0.0f;
				break;
			}

			Sleep(35);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Exception in animation thread.");
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, e.what());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception thread.");
	}
	animating = false;
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation complete.");
}

void renderSectorInfo() {

	// TODO possible skips for render:
	// - when in map view
	// - when in competitive modes
	// - when idk lmao
	// TODO possible skips into settings?

	MapData* currentMap = currentMapService.getCurrentMap();
	if (currentMap == nullptr) return;
	if (currentSectorId != currentMap->currentSector.id) {
		currentSectorId = currentMap->currentSector.id;
		cancelAnimation = true;
		if (animationThread.joinable()) {
			animationThread.join();
			animating = false;
		}
		// we need to render! start the animation thread if not animating already
		if (!animating && !animationThread.joinable()) {
			animationThread = std::thread(fade);
			animationThread.detach();
		}
	}

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;
	// Make the next Window go over the entire screen for easier calculations
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	// And make sure to disable all interaction with it properly *cough*
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

		std::vector<std::string> strings;
		if (displayContinent) {
			strings.push_back(currentMap->continentName);
		}
		if (displayRegion) {
			strings.push_back(currentMap->regionName);
		}
		if (displayMap) {
			strings.push_back(currentMap->name);
		}
		if (strings.size() > 0) {
			std::string miniText = std::accumulate(std::next(strings.begin()), strings.end(), strings[0], [](std::string a, std::string b) {
				return a + " | " + b;
				});
			ImGui::PushFont((ImFont*)NexusLink->Font);
			centerText(miniText, 3.8f);
			ImGui::PopFont();
		}

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
	ImGuiIO& io = ImGui::GetIO();

	if(ImGui::Begin("TyrianRegionsDebug"))
	{
		ImGui::PushFont((ImFont*)NexusLink->Font);

		ImGui::Text("Player information");
		ImGui::Text(("GlobalX: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.X)).c_str());
		ImGui::Text(("GlobalY: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.X)).c_str());

		ImGui::Separator();
		ImGui::Text("Mumble Information");
		ImGui::Text(("Map Id: " + std::to_string(MumbleLink->Context.MapID)).c_str());
		ImGui::Text(("Competitive: " + std::to_string(MumbleLink->Context.IsCompetitive)).c_str());

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
		ImGui::PopFont();
	
		ImGui::Separator();
		std::string title = "Loaded fonts: " + std::to_string(io.Fonts->Fonts.size());
		if (ImGui::CollapsingHeader(title.c_str())) {
			
			for (auto font : io.Fonts->Fonts) {
				
				if (font->IsLoaded()) {

					ImGui::PushFont(font);
					std::string fontName = "Font name: " + std::string(font->ConfigData->Name) + ", Size : " + std::to_string(font->FontSize);
					ImGui::Text(fontName.c_str());
					ImGui::Text("ABCDEFGHIJKLMNOPQRSTUVWXYZ_abdefghijklmnopqrstuvwxyz");
					ImGui::PopFont();
				}
				else {

					ImGui::PushFont((ImFont*)NexusLink->Font);
					const char* debugname = font->GetDebugName();
					std::string message = "Error: Font not loaded: " + std::string(debugname);
					ImGui::Text(message.c_str());
					ImGui::PopFont();
				}
				ImGui::Separator();
			}
		}
	}
	ImGui::End();
}