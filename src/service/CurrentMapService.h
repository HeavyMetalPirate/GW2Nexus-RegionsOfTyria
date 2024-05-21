#ifndef CURRENT_MAP_SERVICE_H
#define CURRENT_MAP_SERVICE_H

#include "../Globals.h"

/// <summary>
/// Struct for displaying a sector
/// </summary>
struct SectorData {
	int id;
	std::string name;
};
/// <summary>
/// Struct for displaying a map
/// </summary>
struct MapData {
	int id;
	std::string name;
	int regionId;
	std::string regionName;
	int continentId;
	std::string continentName;

	SectorData currentSector;
};

class CurrentMapService {
public:

	CurrentMapService();
	~CurrentMapService();

	/// <summary>
	/// Gets the current map data based on Mumble Link data
	/// </summary>
	/// <returns></returns>
	MapData* getCurrentMap();
private:
	MapData* currentMap;
};


#endif