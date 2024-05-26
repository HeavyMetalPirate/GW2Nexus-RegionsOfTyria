#include "AddonRenderer.h"
#include <fstream>


using json = nlohmann::json;

/* Proto */
void centerText(std::string text, float textY, float opacityOverride);
void fade();

/* Render subfunctions */
void renderSampleInfo();
void renderSectorInfo();
void renderInfo(std::string continent, std::string region, std::string map, std::string sector, float opacityOverride);
void renderDebugInfo();

std::thread animationThread;
std::mutex animMutex;

bool cancelAnimation = false;
bool animating;
float opacity = 0.0f;

CurrentMapService currentMapService = CurrentMapService();
int currentSectorId = -1;
int currentMapId = -1;

Renderer::Renderer() {}
Renderer::~Renderer() {}

void Renderer::unload() {
	unloading = true;
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
	renderSampleInfo();
	renderSectorInfo();
	renderDebugInfo();
}

void centerText(std::string text, float textY, float opacityOverride) {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;

	// Center Text voodoo
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	float textX = (windowSize.x - textSize.x) / 2.0f;

	ImGui::SetCursorScreenPos(ImVec2(textX + 2, textY + 2));
	ImGui::TextColored(ImVec4(0, 0, 0, opacityOverride), text.c_str()); // shadow
	ImGui::SetCursorPos(ImVec2(textX, textY));
	ImGui::TextColored(ImVec4(settings.fontColor[0], settings.fontColor[1], settings.fontColor[2], opacityOverride), text.c_str()); // text
}


void cancelCurrentAnimation() {
	opacity = 0.0f;
	animating = false;
	cancelAnimation = false;
	//APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation cancelled");
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
	std::lock_guard<std::mutex> lock(animMutex); // Ensures single-thread access

	if (animating) return; // we are already animating, so stop it now
	//APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation started.");
	animating = true;
	try {
		// fade in
		while (true)
		{
			if (unloading || cancelAnimation) {
				cancelCurrentAnimation();
				return;
			}
			opacity += 0.05f;
			if (opacity > 1) {
				opacity = 1.0f;
				break;
			}

			Sleep(35);
		}
		// Stay sharp
		for (int i = 0; i < 3000;i++) {
			if (unloading || cancelAnimation) {
				cancelCurrentAnimation();
				return;
			}
			Sleep(1); // sleep first so the text stays a little
		}
		// fade out
		while (true)
		{
			if (unloading || cancelAnimation) {
				cancelCurrentAnimation();
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
	cancelAnimation = false;
	animating = false;
	//APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation thread complete.");
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	size_t startPos = 0;
	while ((startPos = str.find(from, startPos)) != std::string::npos) {
		str.replace(startPos, from.length(), to);
		startPos += to.length(); // Move past the replaced substring
	}
}

void renderSampleInfo() {
	if (!showTemplate) return;
	renderInfo("Continent", "Region", "Map", "Sector", 1.0f);
}

void renderSectorInfo() {

	// TODO possible skips for render:
	// - when in map view
	// - when in competitive modes
	// - when idk lmao
	// TODO possible skips into settings?

	MapData* currentMap = currentMapService.getCurrentMap();
	if (currentMap == nullptr) return;

	if (currentSectorId != currentMap->currentSector.id // sector change
		|| currentMapId != currentMap->id) { // map change, we could technically end up in a sector with same id since I parse unknown sectors as -1
	
		currentMapId = currentMap->id;
		currentSectorId = currentMap->currentSector.id;

		// check if we are currently animating and cancel the animation, yeah?
		if (animating) {
			cancelAnimation = true;			
			while (animating) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			//APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Animation cancelled successfully.");
		}
		//APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Starting new animation thread.");
		cancelAnimation = false;
		animationThread = std::thread(fade);
		animationThread.detach();		
	}
	
	if (!animating) return;
	renderInfo(currentMap->continentName, currentMap->regionName, currentMap->name, currentMap->currentSector.name, opacity);
}

void renderInfo(std::string continent, std::string region, std::string map, std::string sector, float opacityOverride) {
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
		// Figure out the texts to display based on the templates provided
		// TODO FIXME if displayFormatSmall or Large are empty, revert to a default?
		// maybe only if both are empty because I could for example want the small text be empty?
		// maybe also only for largeText. gonna ponder over this.
		std::string smallText = std::string(settings.displayFormatSmall);
		// Before anyone flames me for not doing lower/upper case here: the display format might have user defined text that shouldn't be casted
		// and I am way too lazy to parse only my placeholders to u/lcase.
		replaceAll(smallText, "@c", continent);
		replaceAll(smallText, "@C", continent);
		replaceAll(smallText, "@r", region);
		replaceAll(smallText, "@R", region);
		replaceAll(smallText, "@m", map);
		replaceAll(smallText, "@M", map);
		replaceAll(smallText, "@s", sector);
		replaceAll(smallText, "@S", sector);

		std::string largeText = std::string(settings.displayFormatLarge);
		if (largeText.empty()) largeText = "@s"; // default if empty
		// See reasoning above. Still too lazy, nothing has changed in the past 5 or so seconds.
		replaceAll(largeText, "@c", continent);
		replaceAll(largeText, "@C", continent);
		replaceAll(largeText, "@r", region);
		replaceAll(largeText, "@R", region);
		replaceAll(largeText, "@m", map);
		replaceAll(largeText, "@M", map);
		replaceAll(largeText, "@s", sector);
		replaceAll(largeText, "@S", sector);

		// Large Text - change scale just for this
		ImFont* font = (ImFont*)NexusLink->FontBig;
		float originalFontScale = font->Scale;
		font->Scale = settings.fontScale;
		ImGui::PushFont(font);
		centerText(largeText, settings.verticalPosition, opacityOverride);
		font->Scale = originalFontScale;
		ImGui::PopFont();

		// Small Text
		ImFont* fontSmall = (ImFont*)NexusLink->Font;
		originalFontScale = fontSmall->Scale;
		fontSmall->Scale = (settings.fontScale *2/3); // scale it a little smaller
		ImGui::PushFont(fontSmall);
		centerText(smallText, settings.verticalPosition - settings.spacing, opacityOverride);
		fontSmall->Scale = originalFontScale;
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