#include "CurrentMapService.h"

CurrentMapService::CurrentMapService() {}
CurrentMapService::~CurrentMapService() {}

MapData* CurrentMapService::getCurrentMap() {

	// currently broken in competitive modes due to mumble not delivering global coordinates. disable until I can properly calculate it
	if (MumbleLink->Context.IsCompetitive) {
		return nullptr;
	}
	if (!NexusLink->IsGameplay) {
		return nullptr; // do not update while not in gameplay
	}
	
	int currentMapId = MumbleLink->Context.MapID;
	auto position = MumbleLink->AvatarPosition;

	std::string localestr = GetLocaleAsString(locale);
	
	gw2::map* map = mapInventory->getMapInfo(localestr, currentMapId);
	if (map == nullptr) return nullptr;

	SectorData currentSector = SectorData();
	// To detect the current sector we can iterate over the sectors and check if the position is within all boundaries;
	// MumbleLink->Context.Compass.PlayerPosition has the global position

	// we need at least 3 
	if (map->sectors.size() < 3) {
		currentSector.id = -1;
		currentSector.name = map->name; // use map name instead
	}
	else {
		float x = MumbleLink->Context.Compass.PlayerPosition.X;
		float y = MumbleLink->Context.Compass.PlayerPosition.Y;
		for (auto sector : map->sectors) {
			bool inside = false;
			size_t count = sector.second.bounds.size();

			//Ray - Casting - Algorithm, sponsored by ChatGPT
			for (int i = 0, j = count - 1; i < count; j = i++) {
				if ((sector.second.bounds[i].y > y) != (sector.second.bounds[j].y > y) &&
					(x < (sector.second.bounds[j].x - sector.second.bounds[i].x) * (y - sector.second.bounds[i].y) / (sector.second.bounds[j].y - sector.second.bounds[i].y) + sector.second.bounds[i].x)) {
					inside = !inside;
				}
			}
			if (inside) {
				currentSector.id = sector.second.id;
				currentSector.name = sector.second.name;
				break;
			}
		}
	}
	if(currentMap != nullptr)
		if (map->id == currentMap->id 
			&& currentSector.id == currentMap->currentSector.id) 
			return currentMap; // no change, return same pointer

	currentMap = new MapData();
	currentMap->id = map->id;
	currentMap->name = map->name;
	currentMap->regionId = map->regionId;
	currentMap->regionName = map->regionName;
	currentMap->continentId = map->continentId;
	currentMap->continentName = map->continentName;
	currentMap->currentSector = currentSector;

	return currentMap;
}