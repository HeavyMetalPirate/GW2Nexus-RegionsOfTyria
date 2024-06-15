///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// This code is licensed under the MIT license.
/// You should have received a copy of the license along with this source file.
/// You may obtain a copy of the license at: https://opensource.org/license/MIT
/// 
/// Name         :  entry.cpp
/// Description  :  Simple example of a Nexus addon implementation.
///----------------------------------------------------------------------------------------------------

#include "service/MapLoaderService.h"
#include "service/AddonRenderer.h"
#include "service/AddonInitalize.h"

#include "Globals.h"

#include <cstring> // For strcpy_s

/* proto */

// Addon loading
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
// Rendering
void AddonRender();
void AddonOptions();
void AddonShortcut();
void PreRender();
void PostRender();
// Fonts
void ReceiveFont(const char* aIdentifier, void* aFont);
void loadFonts();
void loadFontsThreaded();
void releaseFonts();
// Keybinds
void ProcessKeybind(const char* aIdentifer);
void HandleIdentityChanged(void* anEventArgs);
// Settings
void LoadSettings();
void StoreSettings();
std::string getAddonFolder();

/* globals */
HMODULE hSelf				   = nullptr;
AddonDefinition AddonDef	   = {};
AddonAPI* APIDefs			   = nullptr;
NexusLinkData* NexusLink	   = nullptr;
Mumble::Data* MumbleLink	   = nullptr;
MapInventory* mapInventory     = nullptr; 
WorldInventory* worldInventory = nullptr;
gw2api::wvw::match* match      = nullptr;

bool unloading = false;

/* settings */
bool showDebug = false;
bool showTemplate[6] = { false, false, false, false, false, false };
int templateRace = 0;
bool showServerSelection = false;

// local temps
std::string characterName = "";

Settings settings = {
	0, // FontsVersion
	Locale::En, // localse
	// Racial Font settings [6] 
	{
		{
			"Generic", 28.0f, "@c / @r / @m", 72.0f, "@s", 300.0f, 25.0f, 1.0f,	{255,255,255}, "@s", 20.0f, {255,255,255}
		}, {
			"Asura", 28.0f,	"@c / @r / @m",	72.0f, "@s", 300.0f, 25.0f,	1.0f, {255,255,255}, "@s", 20.0f, {255,255,255}
		}, {
			"Charr", 28.0f,	"@c / @r / @m",	72.0f, "@s", 300.0f, 25.0f,	1.0f, {255,255,255}, "@s", 20.0f, {255,255,255}
		}, {
			"Human", 28.0f,	"@c / @r / @m",	72.0f, "@s", 300.0f, 25.0f, 1.0f, {255,255,255}, "@s", 20.0f, {255,255,255}
		}, {
			"Norn", 28.0f, "@c / @r / @m", 72.0f, "@s",	300.0f,	25.0f, 1.0f, {255,255,255}, "@s", 20.0f, {255,255,255}
		}, {
			"Sylvari", 28.0f, "@c / @r / @m", 72.0f, "@s", 300.0f, 25.0f, 1.0f,	{255,255,255}, "@s", 20.0f, {255,255,255}
		}
	}, // Racial Font settings end
	-1, // wvw world
	0, // fontMode
	false, // disableAnimations
	true, // enablePopup
	false, // widgetEnabled
	100.0f, // widgetPosX
	100.0f, // widgetPosY
	200.0f, // widgetWidt
	0.8f, // widgetBackgroundOpacity

	// deprecated Legacy settings
	"@c / @r / @m",
	"@s",
	300.0f,
	25.0f,
	1.5f,
	{255,255,255}
};

// local temps
char displayFormatSmallBuffer[100] = "";
char displayFormatLargeBuffer[100] = "";

std::mutex identityMutex;

/* services */
Renderer renderer;
MapLoaderService mapLoader;

///----------------------------------------------------------------------------------------------------
/// DllMain:
/// 	Main entry point for DLL.
/// 	We are not interested in this, all we get is our own HMODULE in case we need it.
///----------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH: hSelf = hModule; break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
	}
	return TRUE;
}

///----------------------------------------------------------------------------------------------------
/// GetAddonDef:
/// 	Export needed to give Nexus information about the addon.
///----------------------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -126452345; // set to random unused negative integer
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Regions Of Tyria";
	AddonDef.Version.Major = 1;
	AddonDef.Version.Minor = 4;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "HeavyMetalPirate.2695";
	AddonDef.Description = "Displays the current sector whenever you cross borders, much like your favorite (MMO)RPG does.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/HeavyMetalPirate/GW2Nexus-RegionsOfTyria";

	return &AddonDef;
}

///----------------------------------------------------------------------------------------------------
/// AddonLoad:
/// 	Load function for the addon, will receive a pointer to the API.
/// 	(You probably want to store it.)
///----------------------------------------------------------------------------------------------------
void AddonLoad(AddonAPI* aApi)
{
	APIDefs = aApi; // store the api somewhere easily accessible

	ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext); // cast to ImGuiContext*
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

	NexusLink = (NexusLinkData*)APIDefs->GetResource("DL_NEXUS_LINK");
	MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");

	// TODO clean this code up at some point, keep it for now so you remember how to do this
	//APIDefs->AddShortcut("QA_MYFIRSTADDON", "ICON_PIKACHU", "ICON_JAKE", KB_MFA, "ASDF!");
	APIDefs->RegisterKeybindWithString(KB_MFA, ProcessKeybind, "CTRL+ALT+SHIFT+L");
	APIDefs->AddSimpleShortcut(ADDON_NAME_LONG, AddonShortcut);

	// Unpack the addon resources to the addon data
	unpackResources();
	LoadSettings();

	// if fonts version is not yet set, do the magic
	if (settings.fontsVersion == 0) {
		UnpackFonts(true);
		settings.fontsVersion = fontsVersion;
	}

	// Register Events
	APIDefs->SubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	renderer = Renderer();
	mapLoader = MapLoaderService();
	mapInventory = new MapInventory();
	worldInventory = new WorldInventory();

	// Start filling the inventory in the background
	mapLoader.initializeMapStorage();

	// Initialize the custom fonts
	loadFonts();

	// Add an options window and a regular render callback - always do this at the end I guess
	APIDefs->RegisterRender(ERenderType_PreRender, PreRender);
	APIDefs->RegisterRender(ERenderType_PostRender, PostRender);
	APIDefs->RegisterRender(ERenderType_OptionsRender, AddonOptions);
	APIDefs->RegisterRender(ERenderType_Render, AddonRender);
	// Log initialize
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#00ff00>Initialize complete.</c>");
}

void ReceiveFont(const char* aIdentifier, void* aFont) {
	std::string str = aIdentifier;

	if (aFont == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME,("Received nullptr for font " + std::string(aIdentifier)).c_str());
		return;
	}

	if (str == "ROT_FONT_GENERIC_SMALL")
	{
		renderer.registerFont(fontNameGenericSmall, (ImFont*)aFont);
	} 
	else if (str == "ROT_FONT_GENERIC_LARGE")
	{
		renderer.registerFont(fontNameGenericLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_GENERIC_WIDGET")
	{
		renderer.registerFont(fontNameGenericWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_SMALL")
	{
		renderer.registerFont(fontNameAsuraSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_LARGE")
	{
		renderer.registerFont(fontNameAsuraLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_WIDGET")
	{
		renderer.registerFont(fontNameAsuraWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_SMALL")
	{
		renderer.registerFont(fontNameCharrSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_LARGE")
	{
		renderer.registerFont(fontNameCharrLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_WIDGET")
	{
		renderer.registerFont(fontNameCharrWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_SMALL")
	{
		renderer.registerFont(fontNameHumanSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_LARGE")
	{
		renderer.registerFont(fontNameHumanLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_WIDGET")
	{
		renderer.registerFont(fontNameHumanWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_SMALL")
	{
		renderer.registerFont(fontNameNornSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_LARGE")
	{
		renderer.registerFont(fontNameNornLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_WIDGET")
	{
		renderer.registerFont(fontNameNornWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_SMALL")
	{
		renderer.registerFont(fontNameSylvariSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_LARGE")
	{
		renderer.registerFont(fontNameSylvariLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_WIDGET")
	{
		renderer.registerFont(fontNameSylvariWidget, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_GENERIC_ANIM_SMALL") {
		renderer.registerFont(fontNameGenericAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_GENERIC_ANIM_LARGE") {
		renderer.registerFont(fontNameGenericAnimLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_ANIM_SMALL") {
		renderer.registerFont(fontNameAsuraAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_ANIM_LARGE") {
		renderer.registerFont(fontNameAsuraAnimLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_ANIM_SMALL") {
		renderer.registerFont(fontNameCharrAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_ANIM_LARGE") {
		renderer.registerFont(fontNameCharrAnimLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_ANIM_SMALL") {
		renderer.registerFont(fontNameHumanAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_ANIM_LARGE") {
		renderer.registerFont(fontNameHumanAnimLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_ANIM_SMALL") {
		renderer.registerFont(fontNameNornAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_ANIM_LARGE") {
		renderer.registerFont(fontNameNornAnimLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_ANIM_SMALL") {
		renderer.registerFont(fontNameSylvariAnimSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_ANIM_LARGE") {
		renderer.registerFont(fontNameSylvariAnimLarge, (ImFont*)aFont);
	}
}

///----------------------------------------------------------------------------------------------------
/// AddonUnload:
/// 	Everything you registered in AddonLoad, you should "undo" here.
///----------------------------------------------------------------------------------------------------
void AddonUnload()
{
	// Disable everything that listens to this global
	unloading = true;
	mapLoader.unload();
	renderer.unload();

	//APIDefs->RemoveShortcut("QA_MYFIRSTADDON");
	APIDefs->DeregisterKeybind(KB_MFA);
	APIDefs->RemoveSimpleShortcut(ADDON_NAME_LONG);

	APIDefs->UnsubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	APIDefs->DeregisterRender(PreRender);
	APIDefs->DeregisterRender(PostRender);
	APIDefs->DeregisterRender(AddonRender);
	APIDefs->DeregisterRender(AddonOptions);

	releaseFonts();

	StoreSettings();

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#ff0000>Signing off</c>, it was an honor commander.");
}

/// <summary>
/// PreRender Functionality
/// </summary>
void PreRender() {
	renderer.preRender(ImGui::GetIO());
}

void PostRender() {
	renderer.postRender(ImGui::GetIO());
}

///----------------------------------------------------------------------------------------------------
/// AddonRender:
/// 	Called every frame. Safe to render any ImGui.
/// 	You can control visibility on loading screens with NexusLink->IsGameplay.
///----------------------------------------------------------------------------------------------------
void AddonRender()
{
	renderer.render();

	// Fonts upgrade dialogue
	if (settings.fontsVersion < fontsVersion) {
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_AlwaysAutoResize;
		if (ImGui::Begin("Font version upgraded"), nullptr, flags) {
			ImGui::TextWrapped("Fonts for Regions of Tyria have been updated to a new version. Do you want to upgrade your local installation?");
			ImGui::TextColored({ 255,0,0,1 }, "Warning: Upgrading the fonts will overwrite changes you made to the font files!");
			ImGui::TextWrapped("You can backup your current fonts by copying the TTF files in your <GW2-Install>/addons/TyrianRegions folder.");

			if (ImGui::Button("Yes, I want to upgrade")) {
				UnpackFonts(true);
				releaseFonts();
				loadFontsThreaded();

				settings.fontsVersion = fontsVersion;
			}
			ImGui::SameLine();
			if (ImGui::Button("No, I want to keep the current fonts")) {
				settings.fontsVersion = fontsVersion;
			}

			ImGui::TextWrapped("Note: you can always upgrade the fonts in the options if you choose to do so at a later point.");

			ImGui::End();
		}
	}
}

///----------------------------------------------------------------------------------------------------
/// AddonOptions:
/// 	Basically an ImGui callback that doesn't need its own Begin/End calls.
///----------------------------------------------------------------------------------------------------
/// 
/// 
int InputTextFilterNumbers(ImGuiInputTextCallbackData* data)
{
	if (data->EventChar < 256 && strchr("0123456789", (char)data->EventChar))
		return 0;
	return 1;
}


void AddonOptions()
{
	ImGui::Separator();
	ImGui::Text("Locale");
	ImGui::Text("");
	for (auto item : localeItems) {
		bool selected = settings.locale == item.value;
		ImGui::SameLine();
		if (ImGui::Checkbox(item.description.c_str(), &selected))
		{
			if (selected)
			{
				settings.locale = item.value;
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Features");

	ImGui::Checkbox("Enable Popup Text", &settings.enablePopup);
	ImGui::SameLine();
	ImGui::Checkbox("Disable animations", &settings.disableAnimations);
	
	ImGui::Checkbox("Enable mini widget", &settings.widgetEnabled);
	ImGui::DragFloat("Widget Position (X)", &settings.widgetPositionX, 1.0f, 0, ImGui::GetIO().DisplaySize.x, "%.2f");
	ImGui::DragFloat("Widget Position (Y)", &settings.widgetPositionY, 1.0f, 0, ImGui::GetIO().DisplaySize.y, "%.2f");
	ImGui::DragFloat("Widget Width", &settings.widgetWidth, 1.0f, 1, ImGui::GetIO().DisplaySize.x / 2);
	ImGui::DragFloat("Widget Background", &settings.widgetBackgroundOpacity, 0.05f, 0.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("Display & Styling");

	static const char* comboBoxItems[7];
	comboBoxItems[0] = "Use font depending on race";
	comboBoxItems[1] = "Use Generic font everywhere";
	comboBoxItems[2] = "Use Asuran font everywhere";
	comboBoxItems[3] = "Use Charr font everywhere";
	comboBoxItems[4] = "Use Human font everywhere";
	comboBoxItems[5] = "Use Norn font everywhere";
	comboBoxItems[6] = "Use Sylvari font everywhere";

	if (ImGui::Combo("##combo", &settings.fontMode, comboBoxItems, IM_ARRAYSIZE(comboBoxItems))) {
		renderer.updateFontSettings();
	}
	
	ImGui::Separator();
	
	ImGui::Text("Placeholders: @c = Continent, @r = Region, @m = Map, @s = Sector");
	if (ImGui::BeginTabBar("##Tabs")) {
		int i = -1; // counter variable for templateRace index
		for (auto& fs : settings.fontSettings) {
			++i;
			if (ImGui::BeginTabItem(fs.race.c_str())) {

				if (ImGui::Checkbox("Show sample text", &showTemplate[i])) {
					if (showTemplate[i]) {
						// Uncheck all other checkboxes
						for (int j = 0; j < 6; ++j) {
							if (j != i) showTemplate[j] = false;
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Copy Settings from...")) {
					ImGui::OpenPopup("Copy From...");
				}
				if (ImGui::BeginPopup("Copy From...")) {
					for (int j = 0; j < 6; ++j) {
						if (i != j) {
							if (ImGui::Selectable(settings.fontSettings[j].race.c_str())) {
								fs.displayFormatLarge = settings.fontSettings[j].displayFormatLarge;
								fs.displayFormatSmall = settings.fontSettings[j].displayFormatSmall;
								fs.largeFontSize = settings.fontSettings[j].largeFontSize;
								fs.smallFontSize = settings.fontSettings[j].smallFontSize;
								fs.spacing = settings.fontSettings[j].spacing;
								fs.verticalPosition = settings.fontSettings[j].verticalPosition;
								fs.fontColor[0] = settings.fontSettings[j].fontColor[0];
								fs.fontColor[1] = settings.fontSettings[j].fontColor[1];
								fs.fontColor[2] = settings.fontSettings[j].fontColor[2];

							}
						}
					}
					ImGui::EndPopup();
				}

				char bufferSmall[256];
				strncpy_s(bufferSmall, fs.displayFormatSmall.c_str(), sizeof(bufferSmall));
				if (ImGui::InputText("Display Format Small", bufferSmall, sizeof(bufferSmall))) {
					fs.displayFormatSmall = bufferSmall;
				}
				char bufferLarge[256];
				strncpy_s(bufferLarge, fs.displayFormatLarge.c_str(), sizeof(bufferLarge));
				if (ImGui::InputText("Display Format Large", bufferLarge, sizeof(bufferLarge))) {
					fs.displayFormatLarge = bufferLarge;
				}

				ImGui::DragFloat("Vertical Position", &fs.verticalPosition, 1.0f, 0.0f, ImGui::GetIO().DisplaySize.y);
				ImGui::DragFloat("Spacing", &fs.spacing, 1.0f, -300.0f, 300.0f);

				ImGui::TextWrapped("Attention: changing font sizes requires reloading the fonts to apply the changes.");
				ImGui::Text("Use the button below to reload the fonts.");
				ImGui::InputFloat("Small Font Size", &fs.smallFontSize);
				ImGui::InputFloat("Large Font Size", &fs.largeFontSize);
				ImGui::ColorEdit3("Font Color", fs.fontColor);

				ImGui::Separator();
				ImGui::Text("Widget Settings");
				char bufferWidget[256];
				strncpy_s(bufferWidget, fs.widgetDisplayFormat.c_str(), sizeof(bufferWidget));
				if (ImGui::InputText("Widget Display Format", bufferWidget, sizeof(bufferWidget))) {
					fs.widgetDisplayFormat = bufferWidget;
				}
				ImGui::InputFloat("Widget Font Size", &fs.widgetFontSize);
				ImGui::ColorEdit3("Widget Font Color", fs.widgetFontColor);

				ImGui::EndTabItem();
				
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::Separator();
	
	if (ImGui::Button("Reload fonts")) {
		// if samples is active, disable samples first and enable afterwards.
		int selectedShowTemplate = -1;
		for (int j = 0; j < 6; ++j) {
			if (showTemplate[j]) selectedShowTemplate = j;
			showTemplate[j] = false;
		}

		APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Font reload requested.");
		releaseFonts(); // release fonts sync
		loadFontsThreaded(); // load fonts async, waiting for unload to be executed

		// reenable selected template display
		if (selectedShowTemplate != -1) {
			showTemplate[selectedShowTemplate] = true;
		}
	}

	ImGui::TextWrapped("Use the button 'Reset fonts to default' to unpack all font files again, overwriting any pre-existing ones in the addon folder.");
	if (ImGui::Button("Reset fonts to default")) {
		APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Font reset requested.");
		UnpackFonts(true);
		releaseFonts();
		loadFontsThreaded();
	}

	ImGui::Separator();
	ImGui::Text("WvW specific settings");
	gw2api::worlds::world* current = worldInventory->getWorld(GetLocaleAsString(settings.locale), settings.worldId);

	if (current == nullptr) {
		ImGui::Text("No WvW world selected as home.");
	}
	else {
		ImGui::Text(("Current world selection: " + current->name).c_str());
	}
	if (ImGui::Button("Server Selection")) {
		showServerSelection = !showServerSelection;
	}

	if (showServerSelection) {
		if (ImGui::CollapsingHeader("US Servers")) {
			const int columns = 6; // Number of columns in the grid
			if (ImGui::BeginTable("US_Servers_Table", columns)) {
				int i = 0;
				for (auto w : worldInventory->getAllWorlds(GetLocaleAsString(settings.locale))) {
					if (w->id / 1000 == 1) {

						if (i % columns == 0) {
							ImGui::TableNextRow();
						}
						ImGui::TableNextColumn();

						if (ImGui::Button(w->name.c_str())) {
							settings.worldId = w->id;
							mapLoader.loadWvWMatchFromAPI();
							showServerSelection = false;
						}
						
						i++;
					}
				}
				ImGui::EndTable();
			}
		}
		if (ImGui::CollapsingHeader("EU Servers")) {
			const int columns = 6; // Number of columns in the grid
			if (ImGui::BeginTable("EU_Servers_Table", columns)) {
				int i = 0;
				for (auto w : worldInventory->getAllWorlds(GetLocaleAsString(settings.locale))) {
					if (w->id / 1000 == 2) {

						if (i % columns == 0) {
							ImGui::TableNextRow();
						}
						ImGui::TableNextColumn();

						if (ImGui::Button(w->name.c_str())) {
							settings.worldId = w->id;
							mapLoader.loadWvWMatchFromAPI();
							showServerSelection = false;
						}

						i++;
					}
				}
				ImGui::EndTable();
			}
		}
	}

	// TODO Settings for:
	// - settings to disable a piece of the addon like "no titles but the widget" etc. for when I have more elements to show for
}

void AddonShortcut() {
	// TODO?
	if (ImGui::Checkbox("DebugFrame", &showDebug)) {

	}
}

void ProcessKeybind(const char* aIdentifier)
{
	// TODO clean this up at some point, keep it for now to remember how this works if we need it
	if (strcmp(aIdentifier, KB_MFA) == 0)
	{
		showDebug = !showDebug;
		return;
	}
}

void HandleIdentityChanged(void* anEventArgs) {
	std::lock_guard<std::mutex> lock(identityMutex); // Ensures single-thread access

	Mumble::Identity* identity = (Mumble::Identity*)anEventArgs;
	std::string name(identity->Name);
	if (name != characterName) {
		characterName = std::string(identity->Name);
		if (characterName.empty()) return;
		// trigger sector reset on currentMapService
		renderer.changeCurrentCharacter(characterName);
		renderer.setRacialFont(identity->Race);
		APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Character changed: " + characterName).c_str());
	}
}

void LoadSettings() {
	// Get addon directory
	std::string pathData = getAddonFolder() + "/settings.json";
	if (fs::exists(pathData)) {
		std::ifstream dataFile(pathData);

		if (dataFile.is_open()) {
			json jsonData;
			dataFile >> jsonData;
			dataFile.close();
			// parse settings, yay
			settings = jsonData;
		}
	}
	else {
		// Create new settings!
		StoreSettings();
	}
}

void StoreSettings() {
	json j = settings;

	std::string pathData = getAddonFolder() + "/settings.json";
	std::ofstream outputFile(pathData);
	if (outputFile.is_open()) {
		outputFile << j.dump(4) << std::endl;
		outputFile.close();
	}
	else {
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not store default settings.json - configuration might get lost between loads.");
	}
}

void loadFonts() {
	std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
	APIDefs->AddFontFromFile("ROT_FONT_GENERIC_SMALL", settings.fontSettings[0].smallFontSize, (pathFolder + "/font_generic.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_GENERIC_LARGE", settings.fontSettings[0].largeFontSize, (pathFolder + "/font_generic.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_GENERIC_WIDGET", settings.fontSettings[0].widgetFontSize, (pathFolder + "/font_generic.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_SMALL", settings.fontSettings[1].smallFontSize, (pathFolder + "/font_asura.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_LARGE", settings.fontSettings[1].largeFontSize, (pathFolder + "/font_asura.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_WIDGET", settings.fontSettings[1].widgetFontSize, (pathFolder + "/font_asura.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_SMALL", settings.fontSettings[2].smallFontSize, (pathFolder + "/font_charr.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_LARGE", settings.fontSettings[2].largeFontSize, (pathFolder + "/font_charr.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_WIDGET", settings.fontSettings[2].widgetFontSize, (pathFolder + "/font_charr.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_SMALL", settings.fontSettings[3].smallFontSize, (pathFolder + "/font_human.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_LARGE", settings.fontSettings[3].largeFontSize, (pathFolder + "/font_human.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_WIDGET", settings.fontSettings[3].widgetFontSize, (pathFolder + "/font_human.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_SMALL", settings.fontSettings[4].smallFontSize, (pathFolder + "/font_norn.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_LARGE", settings.fontSettings[4].largeFontSize, (pathFolder + "/font_norn.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_WIDGET", settings.fontSettings[4].widgetFontSize, (pathFolder + "/font_norn.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_SMALL", settings.fontSettings[5].smallFontSize, (pathFolder + "/font_sylvari.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_LARGE", settings.fontSettings[5].largeFontSize, (pathFolder + "/font_sylvari.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_WIDGET", settings.fontSettings[5].widgetFontSize, (pathFolder + "/font_sylvari.ttf").c_str(), ReceiveFont, nullptr);

	// animation fonts as well
	APIDefs->AddFontFromFile("ROT_FONT_GENERIC_ANIM_SMALL", settings.fontSettings[0].smallFontSize, (pathFolder + "/fonts_generic_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_GENERIC_ANIM_LARGE", settings.fontSettings[0].largeFontSize, (pathFolder + "/fonts_generic_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_ANIM_SMALL", settings.fontSettings[1].smallFontSize, (pathFolder + "/fonts_asura_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_ANIM_LARGE", settings.fontSettings[1].largeFontSize, (pathFolder + "/fonts_asura_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_ANIM_SMALL", settings.fontSettings[2].smallFontSize, (pathFolder + "/fonts_charr_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_ANIM_LARGE", settings.fontSettings[2].largeFontSize, (pathFolder + "/fonts_charr_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_ANIM_SMALL", settings.fontSettings[3].smallFontSize, (pathFolder + "/fonts_human_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_ANIM_LARGE", settings.fontSettings[3].largeFontSize, (pathFolder + "/fonts_human_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_ANIM_SMALL", settings.fontSettings[4].smallFontSize, (pathFolder + "/fonts_norn_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_ANIM_LARGE", settings.fontSettings[4].largeFontSize, (pathFolder + "/fonts_norn_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_ANIM_SMALL", settings.fontSettings[5].smallFontSize, (pathFolder + "/fonts_sylvari_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_ANIM_LARGE", settings.fontSettings[5].largeFontSize, (pathFolder + "/fonts_sylvari_anim.ttf").c_str(), ReceiveFont, nullptr);
}

void loadFontsThreaded() {
	// in case we reload fonts during runtime we need to wait for the unload to be executed first
	// that happens before/after render execution. therefore run it threaded with sleeps and waiting
	// for fonts to unload first
	std::thread fontsThread([] {

		// wait for unload
		while (!renderer.isCleared()) {
			Sleep(1);
		}

		// grace period for Nexus to get its shit together
		Sleep(50);
		loadFonts();

		});

	fontsThread.detach();
}

void releaseFonts() {

	APIDefs->ReleaseFont("ROT_FONT_GENERIC_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_GENERIC_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_GENERIC_WIDGET", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_WIDGET", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_WIDGET", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_WIDGET", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_WIDGET", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_WIDGET", ReceiveFont);

	APIDefs->ReleaseFont("ROT_FONT_GENERIC_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_GENERIC_ANIM_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_ANIM_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_ANIM_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_ANIM_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_ANIM_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_ANIM_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_ANIM_LARGE", ReceiveFont);

	renderer.clearFonts();
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Font unload queued successfully.");
}