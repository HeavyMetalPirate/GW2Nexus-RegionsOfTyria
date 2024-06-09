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
bool showTemplate = false;
bool showServerSelection = false;

// local temps
std::string characterName = "";

Settings settings = {
	Locale::En,
	{},
	-1,
	"@c | @r | @m",
	"@s",
	300.0f,
	25.0f,
	1.5f,
	{255,255,255}
};

// local temps
char displayFormatSmallBuffer[100] = "";
char displayFormatLargeBuffer[100] = "";

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
	AddonDef.Version.Minor = 3;
	AddonDef.Version.Build = 0;
	AddonDef.Version.Revision = 1;
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
	//APIDefs->RegisterKeybindWithString(KB_MFA, ProcessKeybind, "CTRL+ALT+SHIFT+L");
	APIDefs->AddSimpleShortcut(ADDON_NAME_LONG, AddonShortcut);

	// Unpack the addon resources to the addon data
	unpackResources();
	LoadSettings();

	// Register Events
	APIDefs->SubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	renderer = Renderer();
	mapLoader = MapLoaderService();
	mapInventory = new MapInventory();
	worldInventory = new WorldInventory();

	// Start filling the inventory in the background
	mapLoader.initializeMapStorage();

	// Initialize the custom fonts
	std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_SMALL", 20.0f, (pathFolder + "/font_asura.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_LARGE", 72.0f, (pathFolder + "/font_asura.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_SMALL", 20.0f, (pathFolder + "/font_charr.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_LARGE", 72.0f, (pathFolder + "/font_charr.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_SMALL", 20.0f, (pathFolder + "/font_human.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_LARGE", 72.0f, (pathFolder + "/font_human.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_SMALL", 20.0f, (pathFolder + "/font_norn.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_LARGE", 72.0f, (pathFolder + "/font_norn.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_SMALL", 20.0f, (pathFolder + "/font_sylvari_v2.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_LARGE", 72.0f, (pathFolder + "/font_sylvari_v2.ttf").c_str(), ReceiveFont, nullptr);
	// animation fonts as well
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_ANIM_SMALL", 20.0f, (pathFolder + "/fonts_asura_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_ASURA_ANIM_LARGE", 72.0f, (pathFolder + "/fonts_asura_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_ANIM_SMALL", 20.0f, (pathFolder + "/fonts_charr_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_CHARR_ANIM_LARGE", 72.0f, (pathFolder + "/fonts_charr_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_ANIM_SMALL", 20.0f, (pathFolder + "/fonts_human_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_HUMAN_ANIM_LARGE", 72.0f, (pathFolder + "/fonts_human_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_ANIM_SMALL", 20.0f, (pathFolder + "/fonts_norn_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_NORN_ANIM_LARGE", 72.0f, (pathFolder + "/fonts_norn_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_ANIM_SMALL", 20.0f, (pathFolder + "/fonts_sylvari_anim.ttf").c_str(), ReceiveFont, nullptr);
	APIDefs->AddFontFromFile("ROT_FONT_SYLVARI_ANIM_LARGE", 72.0f, (pathFolder + "/fonts_sylvari_anim.ttf").c_str(), ReceiveFont, nullptr);


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

	if (str == "ROT_FONT_ASURA_SMALL")
	{
		renderer.registerFont(fontNameAsuraSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_ASURA_LARGE")
	{
		renderer.registerFont(fontNameAsuraLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_SMALL")
	{
		renderer.registerFont(fontNameCharrSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_CHARR_LARGE")
	{
		renderer.registerFont(fontNameCharrLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_SMALL")
	{
		renderer.registerFont(fontNameHumanSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_HUMAN_LARGE")
	{
		renderer.registerFont(fontNameHumanLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_SMALL")
	{
		renderer.registerFont(fontNameNornSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_NORN_LARGE")
	{
		renderer.registerFont(fontNameNornLarge, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_SMALL")
	{
		renderer.registerFont(fontNameSylvariSmall, (ImFont*)aFont);
	}
	else if (str == "ROT_FONT_SYLVARI_LARGE")
	{
		renderer.registerFont(fontNameSylvariLarge, (ImFont*)aFont);
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
	//APIDefs->DeregisterKeybind(KB_MFA);
	APIDefs->RemoveSimpleShortcut(ADDON_NAME_LONG);

	APIDefs->UnsubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	APIDefs->DeregisterRender(PreRender);
	APIDefs->DeregisterRender(PostRender);
	APIDefs->DeregisterRender(AddonRender);
	APIDefs->DeregisterRender(AddonOptions);

	APIDefs->ReleaseFont("ROT_FONT_ASURA_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_ASURA_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_CHARR_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_HUMAN_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_NORN_LARGE", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_SMALL", ReceiveFont);
	APIDefs->ReleaseFont("ROT_FONT_SYLVARI_LARGE", ReceiveFont);

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
	for (auto item : localeItems) {
		bool selected = settings.locale == item.value;
		if (ImGui::Checkbox(item.description.c_str(), &selected))
		{
			if (selected)
			{
				settings.locale = item.value;
			}
		}
	}

	ImGui::Separator();

	ImGui::Text("Display & Styling");
	ImGui::Checkbox("Show sample text", &showTemplate); // TODO maybe move this to the tabs and have the sample text be displayed for that font style
	ImGui::Text("Placeholders: @c = Continent, @r = Region, @m = Map, @s = Sector");
	// TODO settings for "use just one racial font because it's cool" or "use generic font / disable racials"
	// or "disable animations" (wow toxic)
	if (ImGui::BeginTabBar("##Tabs")) {
		for (auto& settings : settings.fontSettings) {
			if (ImGui::BeginTabItem(settings.race.c_str())) {
				
				char bufferSmall[256];
				strncpy_s(bufferSmall, settings.displayFormatSmall.c_str(), sizeof(bufferSmall));
				if (ImGui::InputText("Display Format Small", bufferSmall, sizeof(bufferSmall))) {
					settings.displayFormatSmall = bufferSmall;
				}
				char bufferLarge[256];
				strncpy_s(bufferLarge, settings.displayFormatLarge.c_str(), sizeof(bufferLarge));
				if (ImGui::InputText("Display Format Large", bufferLarge, sizeof(bufferLarge))) {
					settings.displayFormatLarge = bufferLarge;
				}

				ImGui::SliderFloat("Vertical Position", &settings.verticalPosition, 0.0f, ImGui::GetIO().DisplaySize.y);
				ImGui::SliderFloat("Spacing", &settings.spacing, -100.0f, 300.0f);
				ImGui::SliderFloat("Font Scale", &settings.fontScale, 0.5f, 5.0f);
				ImGui::ColorEdit3("Font Color", settings.fontColor);

				// TODO test if this is true once Delta releases the next iteration or the final version of Nexus API 4.
				ImGui::Text("Attention: changing font sizes requires unloading and reloading the addon.");
				ImGui::InputFloat("Small Font Size", &settings.smallFontSize);
				ImGui::InputFloat("Large Font Size", &settings.largeFontSize);

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
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

		return;
	}
}

void HandleIdentityChanged(void* anEventArgs) {
	Mumble::Identity* identity = (Mumble::Identity*)anEventArgs;
	if (identity->Name != characterName.c_str()) {
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

			settings = jsonData;
			settings.displayFormatSmall.copy(displayFormatSmallBuffer, 100);
			settings.displayFormatLarge.copy(displayFormatLargeBuffer, 100);
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

std::string getAddonFolder() {
	std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
	// Create folder if not exist
	if (!fs::exists(pathFolder)) {
		try {
			fs::create_directory(pathFolder);
		}
		catch (const std::exception& e) {
			std::string message = "Could not create addon directory: ";
			message.append(pathFolder);
			APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, message.c_str());

			// Suppress the warning for the unused variable 'e'
			#pragma warning(suppress: 4101)
			e;
		}
	}
	return pathFolder;
}