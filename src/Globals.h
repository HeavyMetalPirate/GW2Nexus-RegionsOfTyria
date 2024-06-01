#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#ifndef STRICT
#define STRICT
#endif // !STRICT

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <numeric>
#include <math.h>

#include "imgui/imgui.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

#include "Constants.h"
#include "Settings.h"
#include "service/MapInventory.h"
#include "service/WorldInventory.h"
#include "entity/GW2API_WvW.h"

extern HMODULE hSelf;
extern AddonAPI* APIDefs;
extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;

extern MapInventory* mapInventory;
extern WorldInventory* worldInventory;
extern gw2api::wvw::match* match;

extern bool unloading;

#endif // GLOBALS_H