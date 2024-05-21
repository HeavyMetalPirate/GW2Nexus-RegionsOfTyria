#ifndef MAP_LOADER_SERVICE_H
#define MAP_LOADER_SERVICE_H
// NOTE: HTTPLIB NEEDS TO BE PUT BEFORE Windows.h!!
//#include "httplib.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>

#include "../Globals.h"
#include "../entity/GW2API_Continents.h"

class MapLoaderService {
public:
	MapLoaderService();
	~MapLoaderService();

	/// <summary>
	/// Startup function to initialize the Map Inventory
	/// </summary>
	void initializeMapStorage();
	void unload();
private:
	//httplib::Result performRequest(std::string uri);

	void loadAllMapsFromApi();
	void loadAllMapsFromStorage();
	void unpackMaps();
};

#endif /* MAP_LOADER_SERVICE_H */