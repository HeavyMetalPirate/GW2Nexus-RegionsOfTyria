#include "CurrentMapService.h"

CurrentMapService::CurrentMapService() {}
CurrentMapService::~CurrentMapService() {}

MapData* CurrentMapService::getCurrentMap() {
	int currentMapId = MumbleLink->Context.MapID;
	auto position = MumbleLink->AvatarPosition;
	MumbleLink->Context;
	
	std::string locale = "en"; // 
	gw2::map* map = mapInventory->getMapInfo(locale, currentMapId);
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
			int count = sector.second.bounds.size();

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

	MapData* mapData = new MapData();
	mapData->id = map->id;
	mapData->name = map->name;
	mapData->regionId = map->regionId;
	mapData->regionName = map->regionName;
	mapData->continentId = map->continentId;
	mapData->continentName = map->continentName;
	mapData->currentSector = currentSector;
	return mapData;
}