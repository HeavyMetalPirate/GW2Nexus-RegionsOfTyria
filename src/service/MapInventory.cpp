#include "MapInventory.h"

MapInventory::MapInventory() {}

void MapInventory::addMap(std::string locale, gw2::map* mapInfo) {
	this->loadedMaps[locale][mapInfo->id] = mapInfo;
}

gw2::map* MapInventory::getMapInfo(std::string locale, int id) {
	if (this->loadedMaps[locale].count(id))
		return this->loadedMaps[locale][id];
	else
		return nullptr;
}

std::map<int, gw2::map*> MapInventory::getLoadedMaps(std::string locale) {
	return this->loadedMaps[locale];
}
