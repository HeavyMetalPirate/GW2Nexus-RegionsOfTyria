#include "AddonRenderer.h"
#include <fstream>

#include "AddonInitalize.h"

using json = nlohmann::json;

/* Proto */
void fade();
void cancelCurrentAnimation();
std::string replacePlaceholderTexts(std::string text);

std::thread animationThread;
std::mutex animMutex;

bool cancelAnimation = false;
bool animating;
float opacity = 0.0f;

CurrentMapService currentMapService = CurrentMapService();
int currentSectorId = -1;
int currentMapId = -1;
std::string currentCharacter = "";
Mumble::ERace currentRace;
RacialFontSettings* fontSettings = nullptr;
bool fontsPicked;

bool fontsLoaded = false;
int expectedFontCount = 24;

Renderer::Renderer() {}
Renderer::~Renderer() {}


void Renderer::changeCurrentCharacter(std::string c) {
	if (currentCharacter != c) {
		currentCharacter = c;
		currentSectorId = -1;
		currentMapId = -1;
		fontsPicked = false;
	}
}

void Renderer::unload() {
	unloading = true;
	if (animationThread.joinable()) {
		animationThread.join();
	}
}

bool Renderer::isCleared() {
	return fonts.size() == 0;
}

void Renderer::clearFonts() {
	// set flag that *hopefully* stops rendering the fonts
	fontsLoaded = false;
	fontsPicked = false;

	// reset the font selection so on the next loop it will default to Nexus fonts fallback
	fontLarge = nullptr;
	fontSmall = nullptr;
	fontAnimLarge = nullptr;
	fontAnimSmall = nullptr;

	// clear the fonts map
	fonts.clear();
}

void Renderer::registerFont(std::string name, ImFont* font) {
#ifndef NDEBUG
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Registering font: " + name).c_str());
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, font->GetDebugName());
#endif
	fonts.emplace(name, font);

	if (fonts.size() == expectedFontCount) {
		APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "All fonts loaded and registered with the renderer.");
		fontsLoaded = true;
	}
}

void Renderer::updateFontSettings() {
	// update wrapper so the options dialog does not need knowledge about currentRace
	setRacialFont(currentRace);
}

void Renderer::setGenericFont() {
	fontSettings = &settings.fontSettings[0];
	fontLarge = fonts[fontNameGenericLarge];
	fontSmall = fonts[fontNameGenericSmall];
	fontAnimLarge = fonts[fontNameGenericAnimLarge];
	fontAnimSmall = fonts[fontNameGenericAnimSmall];
	fontWidget = fonts[fontNameGenericWidget];
}

void Renderer::setRacialFont(Mumble::ERace race) {
	// set default values
	currentRace = race;
	// if we haven't been supplied with fonts leave here to avoid nullpointers
	if (!fontsLoaded) { return; }

	if (settings.fontMode == 1) {
		setGenericFont();
	}
	// if fontmode == 0 => pick font according to race; else fontmode tells us which race the user wants, starting with 2 = asura to 6 = sylvari
	else if ((settings.fontMode == 0 && race == Mumble::ERace::Asura) || settings.fontMode == 2) {
		fontLarge = this->fonts[fontNameAsuraLarge];
		fontSmall = this->fonts[fontNameAsuraSmall];
		fontWidget = this->fonts[fontNameAsuraWidget];

		fontAnimLarge = this->fonts[fontNameAsuraAnimLarge];
		fontAnimSmall = this->fonts[fontNameAsuraAnimSmall];

		fontSettings = &settings.fontSettings[1];
	}
	else if ((settings.fontMode == 0 && race == Mumble::ERace::Charr) || settings.fontMode == 3) {
		fontLarge = this->fonts[fontNameCharrLarge];
		fontSmall = this->fonts[fontNameCharrSmall];
		fontWidget = this->fonts[fontNameCharrWidget];

		fontAnimLarge = this->fonts[fontNameCharrAnimLarge];
		fontAnimSmall = this->fonts[fontNameCharrAnimSmall];

		fontSettings = &settings.fontSettings[2];
	}
	else if ((settings.fontMode == 0 && race == Mumble::ERace::Human) || settings.fontMode == 4) {
		fontLarge = this->fonts[fontNameHumanLarge];
		fontSmall = this->fonts[fontNameHumanSmall];
		fontWidget = this->fonts[fontNameHumanWidget];

		fontAnimLarge = this->fonts[fontNameHumanAnimLarge];
		fontAnimSmall = this->fonts[fontNameHumanAnimSmall];
		
		fontSettings = &settings.fontSettings[3];
	}
	else if ((settings.fontMode == 0 && race == Mumble::ERace::Norn) || settings.fontMode == 5) {
		fontLarge = this->fonts[fontNameNornLarge];
		fontSmall = this->fonts[fontNameNornSmall];
		fontWidget = this->fonts[fontNameNornWidget];

		fontAnimLarge = this->fonts[fontNameNornAnimLarge];
		fontAnimSmall = this->fonts[fontNameNornAnimSmall];

		fontSettings = &settings.fontSettings[4];
	}
	else if ((settings.fontMode == 0 && race == Mumble::ERace::Sylvari) || settings.fontMode == 6) {
		fontLarge = this->fonts[fontNameSylvariLarge];
		fontSmall = this->fonts[fontNameSylvariSmall];
		fontWidget = this->fonts[fontNameSylvariWidget];

		fontAnimLarge = this->fonts[fontNameSylvariAnimLarge];
		fontAnimSmall = this->fonts[fontNameSylvariAnimSmall];

		fontSettings = &settings.fontSettings[5];
	}

#ifndef NDEBUG
	if(fontLarge != nullptr) 
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("FontLarge: " + std::string(fontLarge->GetDebugName())).c_str());
	if(fontSmall != nullptr)
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("FontSmall: " + std::string(fontSmall->GetDebugName())).c_str());
	if (fontWidget != nullptr)
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("FontWidget: " + std::string(fontWidget->GetDebugName())).c_str());
	if(fontAnimLarge != nullptr)
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("FontAnimLarge: " + std::string(fontAnimLarge->GetDebugName())).c_str());
	if(fontAnimSmall != nullptr) 
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("FontAnimSmall: " + std::string(fontAnimSmall->GetDebugName())).c_str());
#endif

	fontsPicked = true;
}

void Renderer::preRender(ImGuiIO& io) {
	// NO OP
}

void Renderer::postRender(ImGuiIO& io) {
	// NO OP
}

void Renderer::render() {
	if (unloading) return;
	renderSampleInfo();
	renderSectorInfo();
	renderMinimapWidget();
	renderDebugInfo();
}

void Renderer::renderSampleInfo() {
	bool renderSample = false;
	int raceIndex = 0;
	for (int i = 0; i < 6; i++) {
		if (showTemplate[i]) {
			raceIndex = i;
			renderSample = true;
			break;
		}
	}
	
	if (!renderSample) return;

	// Store current race
	Mumble::ERace originalPick = currentRace;

	// switch to template race set by options dialog
	switch (raceIndex) {
		case 0: setGenericFont(); break;
		case 1: setRacialFont(Mumble::ERace::Asura); break;
		case 2: setRacialFont(Mumble::ERace::Charr); break;
		case 3: setRacialFont(Mumble::ERace::Human); break;
		case 4: setRacialFont(Mumble::ERace::Norn); break;
		case 5: setRacialFont(Mumble::ERace::Sylvari); break;
		default: setGenericFont();
	}
	// render info with generic texts
	renderInfo("Continent", "Region", "Map", "Sector", 1.0f);

	// reset to the originalPick
	setRacialFont(originalPick);
}

void Renderer::renderMinimapWidget() {
	if (!settings.widgetEnabled) return;
	if (!NexusLink->IsGameplay) return;
	if (MumbleLink->Context.IsMapOpen) return;

	MapData* currentMap = currentMapService.getCurrentMap();
	if (currentMap == nullptr) return;
	if (fontSettings == nullptr && !fontsPicked) {
		setRacialFont(currentRace);
	} else if (!fontsPicked) {
		setRacialFont(currentRace);
	}
	if (fontSettings == nullptr) {
#ifndef NDEBUG
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not find font settings, possibly still initializing.");
#endif
		return;
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar;

	if (fontWidget == nullptr) {
		fontWidget = (ImFont*)NexusLink->FontBig;
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontWidget null, fallback to Nexus default font.");
	}
	else if (!fontWidget->IsLoaded()) {
		fontWidget = (ImFont*)NexusLink->FontBig;
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontWidget not loaded, fallback to Nexus default font.");
	}

	std::string output = fontSettings->widgetDisplayFormat;
	output = replacePlaceholderTexts(output);

	// calculate text size
	ImGui::PushFont(fontWidget);
	ImVec2 textSize = ImGui::CalcTextSize(output.c_str());
	ImGui::PopFont();
	ImVec4 textColor = ImVec4(fontSettings->widgetFontColor[0], fontSettings->widgetFontColor[1], fontSettings->widgetFontColor[2], 1.0f);
	ImVec4 shadowColor = ImVec4(0, 0, 0, 1);

	ImVec2 widgetPos = ImVec2(settings.widgetPositionX, settings.widgetPositionY);
	ImVec2 widgetSize = ImVec2(settings.widgetWidth, textSize.y);
	ImGui::SetNextWindowPos(widgetPos);
	ImGui::SetNextWindowSize(widgetSize);
	ImGui::SetNextWindowBgAlpha(settings.widgetBackgroundOpacity);

	if (ImGui::Begin("MiniSectorWidget", (bool*)0, flags)) {
		ImGui::PushFont(fontWidget);
		float textX = (settings.widgetWidth - textSize.x) / 2.0f;
		ImGui::SetCursorPosX(textX + 1.0f);
		ImGui::SetCursorPosY(1.0f);
		ImGui::TextColored(shadowColor, output.c_str());
		ImGui::SetCursorPosX(textX);
		ImGui::SetCursorPosY(0.0f);
		ImGui::TextColored(textColor, output.c_str());
		ImGui::PopFont();
		ImGui::End();
	}
}

void Renderer::renderSectorInfo() {
	if (!settings.enablePopup) return; 

	MapData* currentMap = currentMapService.getCurrentMap();
	if (currentMap == nullptr) return;
	if (fontSettings == nullptr && !fontsPicked) {
		setRacialFont(currentRace);
	} 
	if (fontSettings == nullptr) {
#ifndef NDEBUG
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not find font settings, possibly still initializing.");
#endif
		return;
	}

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
		}
		cancelAnimation = false;
		animationThread = std::thread(fade);
		animationThread.detach();		
	}
	
	if (!animating) return;
	renderInfo(currentMap->continentName, currentMap->regionName, currentMap->name, currentMap->currentSector.name, opacity);
}

void Renderer::renderInfo(std::string continent, std::string region, std::string map, std::string sector, float opacityOverride) {
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
		std::string smallText = std::string(fontSettings->displayFormatSmall);
		// Before anyone flames me for not doing lower/upper case here: the display format might have user defined text that shouldn't be casted
		// and I am way too lazy to parse only my placeholders to u/lcase.
		/*replaceAll(smallText, "@c", continent);
		replaceAll(smallText, "@C", continent);
		replaceAll(smallText, "@r", region);
		replaceAll(smallText, "@R", region);
		replaceAll(smallText, "@m", map);
		replaceAll(smallText, "@M", map);
		replaceAll(smallText, "@s", sector);
		replaceAll(smallText, "@S", sector);
		*/
		std::string largeText = std::string(fontSettings->displayFormatLarge);
		if (largeText.empty()) largeText = "@s"; // default if empty
		// See reasoning above. Still too lazy, nothing has changed in the past 5 or so seconds.
		/*replaceAll(largeText, "@c", continent);
		replaceAll(largeText, "@C", continent);
		replaceAll(largeText, "@r", region);
		replaceAll(largeText, "@R", region);
		replaceAll(largeText, "@m", map);
		replaceAll(largeText, "@M", map);
		replaceAll(largeText, "@s", sector);
		replaceAll(largeText, "@S", sector);

		// Replace WvW Team placeholders
		// guess what? still to lazy to do it properly. what is maintenance, right?
		std::string redTeamText, blueTeamText, greenTeamText;
		redTeamText = "Red";
		blueTeamText = "Blue";
		greenTeamText = "Green";
		if (match != nullptr) {
			gw2api::worlds::world* redWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.red);
			if (redWorld != nullptr) {
				redTeamText = redWorld->name;
			}

			gw2api::worlds::world* blueWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.blue);
			if (blueWorld != nullptr) {
				blueTeamText = blueWorld->name;
			}
			
			gw2api::worlds::world* greenWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.green);
			if (greenWorld != nullptr) {
				greenTeamText = greenWorld->name;
			}
		}
		replaceAll(smallText, redTeam, redTeamText);
		replaceAll(smallText, blueTeam, blueTeamText);
		replaceAll(smallText, greenTeam, greenTeamText);
		replaceAll(largeText, redTeam, redTeamText);
		replaceAll(largeText, blueTeam, blueTeamText);
		replaceAll(largeText, greenTeam, greenTeamText);
		*/

		smallText = replacePlaceholderTexts(smallText);
		largeText = replacePlaceholderTexts(largeText);

		if (!fontsPicked) {
			setRacialFont(currentRace);
		}
		// whether we picked fonts successfully or not, make sure they're loaded properly and fall back to nexus fonts in case they're not
		// Font fallback
		if (fontLarge == nullptr) {
			fontLarge = (ImFont*)NexusLink->FontBig;
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontLarge null, fallback to Nexus default font.");
		}
		else if (!fontLarge->IsLoaded()) {
			fontLarge = (ImFont*)NexusLink->FontBig;
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontLarge not loaded, fallback to Nexus default font.");
		}
		if (fontSmall == nullptr) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontSmall null, fallback to Nexus default font.");
			fontSmall = (ImFont*)NexusLink->Font;
		}
		else if (!fontSmall->IsLoaded()) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontSmall not loaded, fallback to Nexus default font.");
			fontSmall = (ImFont*)NexusLink->Font;
		}
		// Animation Font flalback
		if (fontAnimLarge == nullptr) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontAnimLarge null, fallback to Nexus default font.");
			fontAnimLarge = (ImFont*)NexusLink->FontBig;
		}
		else if (!fontAnimLarge->IsLoaded()) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontAnimLarge not loaded, fallback to Nexus default font.");
			fontAnimLarge = (ImFont*)NexusLink->FontBig;
		}
		if (fontAnimSmall == nullptr) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontAnimSmall null, fallback to Nexus default font.");
			fontAnimSmall = (ImFont*)NexusLink->Font;
		}
		else if (!fontAnimSmall->IsLoaded()) {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "FontAnimSmall not loaded, fallback to Nexus default font.");
			fontAnimSmall = (ImFont*)NexusLink->Font;
		}

		// Large Text
		centerText(largeText, fontSettings->verticalPosition, opacityOverride);

		// Small Text
		centerTextSmall(smallText, fontSettings->verticalPosition - fontSettings->spacing, opacityOverride);
	}
	ImGui::End();
}

void Renderer::renderDebugInfo() {
	if (!showDebug) return;
	ImGuiIO& io = ImGui::GetIO();

	if(ImGui::Begin("TyrianRegionsDebug"))
	{
		ImGui::PushFont((ImFont*)NexusLink->Font);

		ImGui::Text("Player information");
		ImGui::Text(("GlobalX: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.X)).c_str());
		ImGui::Text(("GlobalY: " + std::to_string(MumbleLink->Context.Compass.PlayerPosition.Y)).c_str());

		gw2::coordinate calcPos = currentMapService.calculatePos();
		ImGui::Text(("CalculatedX: " + std::to_string(calcPos.x)).c_str());
		ImGui::Text(("CalculatedY: " + std::to_string(calcPos.y)).c_str());

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
		ImGui::Separator();
		if (ImGui::CollapsingHeader("WvW Match Data", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (match == nullptr) {
				ImGui::TextColored(ImVec4(255, 0, 0, 1), "No match data loaded!");
			}
			else {
				ImGui::Text(("Id: " + match->id).c_str());
				gw2api::worlds::world* red = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.red);
				gw2api::worlds::world* blue = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.blue);
				gw2api::worlds::world* green = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.green);

				ImGui::Text(("Red world: " + std::to_string(match->worlds.red)).c_str());
				if (red == nullptr) {
					ImGui::TextColored(ImVec4(255, 0, 0, 1), "Red Team unknown!");
				}
				else {
					ImGui::Text(red->name.c_str());
				}

				ImGui::Text(("Blue world: " + std::to_string(match->worlds.blue)).c_str());
				if (blue == nullptr) {
					ImGui::TextColored(ImVec4(255, 0, 0, 1), "Blue Team unknown!");
				}
				else {
					ImGui::Text(blue->name.c_str());
				}

				ImGui::Text(("Green world: " + std::to_string(match->worlds.green)).c_str());
				if (green == nullptr) {
					ImGui::TextColored(ImVec4(255, 0, 0, 1), "Green Team unknown!");
				}
				else {
					ImGui::Text(green->name.c_str());
				}
			}
		}

		ImGui::PopFont();
	
		ImGui::Separator();
		std::string title = "Loaded fonts: " + std::to_string(fonts.size());
		if (ImGui::CollapsingHeader(title.c_str())) {
			
			for (auto font : fonts) {
				
				if (font.second == nullptr) {
					ImGui::PushFont((ImFont*)NexusLink->Font);
					const char* debugname = font.first.c_str();
					std::string message = "Error: Font is nullptr: " + std::string(debugname);
					ImGui::Text(message.c_str());
					ImGui::PopFont();
				}
				else if (font.second->IsLoaded()) {

					std::string fontName = "Font name: " + std::string(font.second->ConfigData->Name) + ", Size : " + std::to_string(font.second->FontSize);
					ImGui::Text(fontName.c_str());
					ImGui::PushFont(font.second);
					ImGui::Text("ABCDEFGHIJKLMNOPQRSTUVWXYZ_abdefghijklmnopqrstuvwxyz");
					ImGui::PopFont();
				}
				else {
					ImGui::PushFont((ImFont*)NexusLink->Font);
					const char* debugname = font.second->GetDebugName();
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

void Renderer::renderTextAnimation(const char* text, float opacityOverride, bool large, bool isShadow) {
	ImFont* main = large ? fontLarge : fontSmall;
	ImFont* secondary = large ? fontAnimLarge : fontAnimSmall;

	if (settings.disableAnimations) {
		secondary = main; // :(
	}

	ImVec4 color = ImVec4(fontSettings->fontColor[0], fontSettings->fontColor[1], fontSettings->fontColor[2], opacityOverride);
	ImVec4 shadow = ImVec4(0, 0, 0, opacityOverride);

	ImVec2 originalCursorPos = ImGui::GetCursorPos();

	float font1Size = main->FontSize;
	float font2Size = secondary->FontSize;

	// Calculate the offset to align secondary to the center of main
	float yOffset = (font1Size - font2Size) * 0.5f;
	float currentX = originalCursorPos.x;

	// Calculate the scaling factor for font2
	float scalingFactor = font1Size / font2Size;
	float originalSecondaryScaling = secondary->Scale;
	secondary->Scale = scalingFactor;

	for (const char* p = text; *p; p++) {

		// Pick font based on opacity; lower opacity more favorably to secondary
		ImFont* selectedFont = (opacityOverride < 1.0f && ((float)rand() / RAND_MAX) > opacityOverride) ? secondary : main;
		
		// Align height to center with main font
		ImGui::PushFont(selectedFont);
		if (selectedFont == secondary) {
			ImVec2 charPos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(charPos.x, charPos.y + yOffset));
		}

		ImGui::PushStyleColor(ImGuiCol_Text, isShadow? shadow : color);
		ImGui::TextUnformatted(p, p + 1);
		ImGui::PopStyleColor();

		// reset position
		if (selectedFont == secondary) {
			ImVec2 charPos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(charPos.x, charPos.y - yOffset));

		}

		ImGui::PopFont();
		
		if (p[1]) {	
			ImVec2 delta = main->CalcTextSizeA(font1Size, FLT_MAX, 0.0f, p, p + 1);
			//currentX += delta.x + (main->GetCharAdvance(p[0])); // shift currentX to (character size if it were main font) + (kerning if it was main font)
			currentX += main->GetCharAdvance(p[0]);
			ImGui::SetCursorPos(ImVec2(currentX, originalCursorPos.y));
		}
	}
	secondary->Scale = originalSecondaryScaling;
}

void Renderer::centerText(std::string text, float textY, float opacityOverride) {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;

	if (text.empty() && currentCharacter == "I Facetank Bosses") {
		text = "Jesus fucking christ Delta, where are you now?";
	}
	else if (text.empty() && currentCharacter != "I Facetank Bosses") {
		text = "The Unknown";
	}

	// Center Text voodoo
	ImGui::PushFont(fontLarge);
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	float textX = (windowSize.x - textSize.x) / 2.0f;
	ImGui::PopFont();

	ImGui::SetCursorScreenPos(ImVec2(textX + 2, textY + 2));
	renderTextAnimation(text.c_str(), opacityOverride, true, true);
	ImGui::SetCursorPos(ImVec2(textX, textY));
	renderTextAnimation(text.c_str(), opacityOverride, true, false);
}
void Renderer::centerTextSmall(std::string text, float textY, float opacityOverride) {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = io.DisplaySize;

	// Center Text voodoo
	ImGui::PushFont(fontSmall);
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	float textX = (windowSize.x - textSize.x) / 2.0f;
	ImGui::PopFont();

	ImGui::SetCursorScreenPos(ImVec2(textX + 2, textY + 2));
	renderTextAnimation(text.c_str(), opacityOverride, false, true);
	ImGui::SetCursorPos(ImVec2(textX, textY));
	renderTextAnimation(text.c_str(), opacityOverride, false, false);
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
#ifndef NDEBUG
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation started.");
#endif
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
		for (int i = 0; i < 3000; i++) {
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
#ifndef NDEBUG
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Animation thread complete.");
#endif
}

std::string replacePlaceholderTexts(std::string text) {
	MapData* currentMap = currentMapService.getCurrentMap();

	replaceAll(text, "@c", currentMap->continentName);
	replaceAll(text, "@C", currentMap->continentName);
	replaceAll(text, "@r", currentMap->regionName);
	replaceAll(text, "@R", currentMap->regionName);
	replaceAll(text, "@m", currentMap->name);
	replaceAll(text, "@M", currentMap->name);
	replaceAll(text, "@s", currentMap->currentSector.name);
	replaceAll(text, "@S", currentMap->currentSector.name);

	// Replace WvW Team placeholders
	// guess what? still to lazy to do it properly. what is maintenance, right?
	std::string redTeamText, blueTeamText, greenTeamText;
	redTeamText = "Red";
	blueTeamText = "Blue";
	greenTeamText = "Green";
	if (match != nullptr) {
		gw2api::worlds::world* redWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.red);
		if (redWorld != nullptr) {
			redTeamText = redWorld->name;
		}

		gw2api::worlds::world* blueWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.blue);
		if (blueWorld != nullptr) {
			blueTeamText = blueWorld->name;
		}

		gw2api::worlds::world* greenWorld = worldInventory->getWorld(GetLocaleAsString(settings.locale), match->worlds.green);
		if (greenWorld != nullptr) {
			greenTeamText = greenWorld->name;
		}
	}
	replaceAll(text, redTeam, redTeamText);
	replaceAll(text, blueTeam, blueTeamText);
	replaceAll(text, greenTeam, greenTeamText);
	
	return text;
}