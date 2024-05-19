#ifndef RENDERER_SERVICE_H
#define RENDERER_SERVICE_H

#include "../Globals.h"
#include "CurrentMapService.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"

class Renderer {
public:
	Renderer();
	~Renderer();
	void preRender(ImGuiIO& io);
	void render();
	void postRender(ImGuiIO& io);
private:
	bool fontsLoaded = false;
	ImFontAtlas* newFontAtlas = new ImFontAtlas();
	ImFontAtlas* defaultFontAtlas = nullptr;

	ImFont* loadFont(ImGuiIO& io, std::string path, float size);
	CurrentMapService currentMapService;
	std::map<std::string, ImFont*> fonts;
};

#endif