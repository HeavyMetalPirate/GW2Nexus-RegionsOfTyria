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

bool MapInventory::isWvWMap(int id) {
	// 38   = EBG
	// 1099 = Red Borderlands
	// 96   = Blue Borderlands
	// 95   = Green Borderlands
	// Possible TODO: the other weird map?
	return(id == 38 || id == 1099 || id == 96 || id == 95);
}
