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

/* proto */
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
void AddonShortcut();
void PreRender();
void PostRender();

void ProcessKeybind(const char* aIdentifer);
void HandleIdentityChanged(void* anEventArgs);

/* globals */
HMODULE hSelf				= nullptr;
AddonDefinition AddonDef	= {};
AddonAPI* APIDefs			= nullptr;
NexusLinkData* NexusLink	= nullptr;
Mumble::Data* MumbleLink	= nullptr;
MapInventory* mapInventory  = nullptr; 

/* settings */
bool showDebug = false;
Locale locale = Locale::Client;

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
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 0;
	AddonDef.Version.Revision = 1;
	AddonDef.Author = "HeavyMetalPirate.2695";
	AddonDef.Description = "Displays the current sector whenever you cross borders, much like your favorite (MMO)RPG does. Spiritual port of the BlishHUD module.";
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

	// Register Events
	APIDefs->SubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	renderer = Renderer();
	mapLoader = MapLoaderService();
	mapInventory = new MapInventory();

	// Start filling the inventory in the background
	mapLoader.initializeMapStorage();

	// Add an options window and a regular render callback - always do this at the end I guess
	APIDefs->RegisterRender(ERenderType_PreRender, PreRender);
	APIDefs->RegisterRender(ERenderType_PostRender, PostRender);
	APIDefs->RegisterRender(ERenderType_Render, AddonRender);
	APIDefs->RegisterRender(ERenderType_OptionsRender, AddonOptions);
	// Log initialize
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#00ff00>Initialize complete.</c>");
}

///----------------------------------------------------------------------------------------------------
/// AddonUnload:
/// 	Everything you registered in AddonLoad, you should "undo" here.
///----------------------------------------------------------------------------------------------------
void AddonUnload()
{
	/* let's clean up after ourselves */
	APIDefs->DeregisterRender(PreRender);
	APIDefs->DeregisterRender(PostRender);
	APIDefs->DeregisterRender(AddonRender);
	APIDefs->DeregisterRender(AddonOptions);

	APIDefs->RemoveShortcut("QA_MYFIRSTADDON");
	APIDefs->DeregisterKeybind(KB_MFA);

	APIDefs->UnsubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);

	mapLoader.~MapLoaderService();
	renderer.~Renderer();
	mapInventory->~MapInventory();

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#ff0000>Signing off</c>, it was an honor commander.");
}

/// <summary>
/// PreRender Functionality
/// </summary>
void PreRender() {
	// TODO bring this in once we figured out custom fonts
	//renderer.preRender(ImGui::GetIO());
}

void PostRender() {
	// TODO probably bring this in once we figured out custom fonts
	//renderer.postRender(ImGui::GetIO());
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
void AddonOptions()
{
	/*
	ImGui::Separator();
	ImGui::Text("My first Nexus addon");
	ImGui::Checkbox("Some setting", &someSetting);
	*/
}

void AddonShortcut() {

	if (ImGui::BeginMenu("Locale")) {
		for (auto item : localeItems) {
			bool selected = locale == item.value;
			if (ImGui::MenuItem(item.description.c_str(), nullptr, &selected))
			{
				if (selected)
				{
					locale = item.value;
				}
			}
		}
		ImGui::EndMenu();
	}
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
}

