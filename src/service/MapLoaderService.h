#ifndef MAP_LOADER_SERVICE_H
#define MAP_LOADER_SERVICE_H

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>

#include "../Globals.h"
#include "../entity/GW2API_Continents.h"
#include "../entity/GW2API_Worlds.h"
#include "../entity/GW2API_WvW.h"

class MapLoaderService {
public:
	MapLoaderService();
	~MapLoaderService();

	/// <summary>
	/// Startup function to initialize the Map Inventory
	/// </summary>
	void initializeMapStorage();

	/// <summary>
	/// Loads WvW Match data from the API.
	/// Requires either an API key or world Id set in the settings.
	/// </summary>
	void loadWvWMatchFromAPI();

	void unload();
private:
	std::string performRequest(std::string uri);

	void loadAllMapsFromApi();
	void loadAllMapsFromStorage();
	void unpackMaps();

	void loadWorldsFromAPI();
};

#endif /* MAP_LOADER_SERVICE_H */