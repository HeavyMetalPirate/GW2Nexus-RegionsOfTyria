#ifndef RENDERER_SERVICE_H
#define RENDERER_SERVICE_H

#include "../Globals.h"
#include "CurrentMapService.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"

#include <thread>

class Renderer {
public:
	Renderer();
	~Renderer();
	void preRender(ImGuiIO& io);
	void render();
	void postRender(ImGuiIO& io);
private:

	/* Fonts shenanigans, currently not working as expected so subject to change */
	bool fontsLoaded = false;

	ImFontAtlas* newFontAtlas = new ImFontAtlas();
	ImFontAtlas* defaultFontAtlas = nullptr;

	ImFont* loadFont(ImGuiIO& io, std::string path, float size);
	std::map<std::string, ImFont*> fonts;
};

#endif