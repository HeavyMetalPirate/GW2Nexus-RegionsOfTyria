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
	void unload();
};

#endif