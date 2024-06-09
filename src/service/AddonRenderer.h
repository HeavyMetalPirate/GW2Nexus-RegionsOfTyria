#ifndef RENDERER_SERVICE_H
#define RENDERER_SERVICE_H

#include "../Globals.h"
#include "CurrentMapService.h"
#include <thread>
#include <mutex>

class Renderer {
public:
	Renderer();
	~Renderer();
	void preRender(ImGuiIO& io);
	void render();
	void postRender(ImGuiIO& io);

	void registerFont(std::string name, ImFont* font);
	void setRacialFont(Mumble::ERace race);
	void setGenericFont();

	void changeCurrentCharacter(std::string currentCharacter);
	void unload();

private:
	std::map<std::string, ImFont*> fonts;
	ImFont* fontLarge = nullptr;
	ImFont* fontSmall = nullptr;
	ImFont* fontAnimLarge = nullptr;
	ImFont* fontAnimSmall = nullptr;

	/* Render subfunctions */
	void renderSampleInfo();
	void renderSectorInfo();
	void renderInfo(std::string continent, std::string region, std::string map, std::string sector, float opacityOverride);
	void renderDebugInfo();

	void renderTextAnimation(const char* text, float opacityOverride, bool large, bool isShadow);
	void centerText(std::string text, float textY, float opacityOverride);
	void centerTextSmall(std::string text, float textY, float opacityOverride);

};

#endif