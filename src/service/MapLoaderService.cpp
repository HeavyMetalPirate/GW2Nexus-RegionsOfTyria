#include "MapLoaderService.h"
#include <Windows.h>
#include "../resource.h"

#include "../ziplib/src/zip.h"
#include "HttpClient.h"

int timeoutCounter = 0;
int retryCounter = 0;

// if I put this as a class field I am getting weird "use of deleted function" errors? wtf.
std::thread initializerThread;

namespace fs = std::filesystem;

static int on_extract_entry(const char* filename, void* arg) {
	static int i = 0;
	int n = *(int*)arg;

	APIDefs->Log(ELogLevel::ELogLevel_DEBUG, ADDON_NAME, (filename));
	return 0;
}

MapLoaderService::MapLoaderService() {}
MapLoaderService::~MapLoaderService() {}

void MapLoaderService::unload() {
	// Ensure the thread has been started.
	if (initializerThread.joinable()) {
		// This will block until the thread has finished.
		initializerThread.join();
	}
}

/// <summary>
/// Utility function to request an URI with yhirose/cpp-httlib.
/// TODO: maybe generalize the return type to decouple from HTTL lib implementation?
/// </summary>
/// <param name="uri">uri to be requested</param>
/// <returns>Result of the call, or nullptr in case of a timeout on the client</returns>
std::string MapLoaderService::performRequest(std::string uri) {	
	std::string requestUri = baseUrl + uri;
	std::string response = HTTPClient::GetRequest(requestUri);

	int retry = 1;
	while (response.empty() && retry < 10) {
		if (unloading) break;
		retry++;
		response = HTTPClient::GetRequest(requestUri);
	}
#ifndef NDEBUG
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Fetched result for " + uri + " after " + std::to_string(retry) + " attempts.").c_str());
#endif
	return response;
}

void MapLoaderService::initializeMapStorage() {
	if (!initializerThread.joinable()) {
		// Move a newly constructed thread to the class member.
		initializerThread = std::thread([&] {
			// Preload some data for WvW maps...
			// Load data for worlds
			loadWorldsFromAPI();
			// Load data from WvW matches
			loadWvWMatchFromAPI();
			// Preload from storage to save time
			loadAllMapsFromStorage();
			// Update from API in case the preloaded data was incomplete
			// This is a longer operation, potentially up to 15 minutes depending on how responsive the API is
			// or could be done in 10 seconds if the API is like super fast? Woah.
			loadAllMapsFromApi(); 
		});
	}

}

void MapLoaderService::loadWorldsFromAPI() {
	for (auto lang : SUPPORTED_LOCAL) {
		std::string worldsResponse = performRequest("/v2/worlds?ids=all&lang=" + lang);
		if (worldsResponse.empty()) {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Loading of Worlds data failed! Sector display in WvW may be incomplete!");
			return;
		}
		json worldsJson = json::parse(worldsResponse);
		std::vector<gw2api::worlds::world> worlds = worldsJson.get<std::vector <gw2api::worlds::world>>();

		for (auto world : worlds)
		{
			gw2api::worlds::world* w = new gw2api::worlds::world(world);
			worldInventory->addWorld(lang, w);
		}
	}
}

void MapLoaderService::loadWvWMatchFromAPI() {
	// Mumble has a bug "right now" that displays shard id for world instead of the actual value...
	// only way to get the right world is /v2/account but that requires providing an API key... 
	// guess we will have fallbacks for a while.
	int worldId = 0;
	 if (settings.worldId != 0) {
		worldId = settings.worldId;
	}
	// if world id hasn't been set until here, we won't complete it today I'm afraid.
	if(worldId == 0) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown world! Sector display in WvW may be incomplete!");
		return;
	}

	std::string matchResponse = performRequest("/v2/wvw/matches?world=" + std::to_string(worldId));
	if (matchResponse.empty()) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Loading of WvW matchup data failed! Sector display in WvW may be incomplete!");
		return;
	}
	try {
		json matchJson = json::parse(matchResponse);
		gw2api::wvw::match m = matchJson;
		// into globals with you!
		match = new gw2api::wvw::match(m);
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Loaded match: " + match->id).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Error parsing WvW match data.");
	}
}

void MapLoaderService::unpackMaps() {

	// get resource from internal bundled resources
	HRSRC hResource = FindResource(hSelf, MAKEINTRESOURCE(IDR_MAPS_ZIP), L"ZIP");
	if (hResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Did not find resource.");
		return;
	}

	// Get Handle of resource
	HGLOBAL hLoadedResource = LoadResource(hSelf, hResource);
	if (hLoadedResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Could not load resource.");
		return;
	}

	// Lock resource
	LPVOID lpResourceData = LockResource(hLoadedResource);
	if (lpResourceData == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Could not lock resource.");
		return;
	}

	std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
	// Create folder if not exist
	if (!fs::exists(pathFolder)) {
		try {
			fs::create_directory(pathFolder);
		}
		catch (const std::exception& e) {
			std::string message = "Could not create addon directory: ";
			message.append(pathFolder);
			APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, message.c_str());

			// Suppress the warning for the unused variable 'e'
			#pragma warning(suppress: 4101)
			e;
		}
	}
	std::string outputPath = pathFolder + "/maps.zip";

	// Open file for writing in binary
	FILE* file = nullptr;
	errno_t err = fopen_s(&file, outputPath.c_str(), "wb");
	if (err != 0 || file == nullptr) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Error trying to write maps.zip (fopen_s)");
		return;
	}

	// Write resource
	size_t resourceSize = SizeofResource(hSelf, hResource);
	fwrite(lpResourceData, 1, resourceSize, file);

	// Close data, free up resources
	fclose(file);
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "maps.zip extracted from module.");
	
	int arg = 2;
	zip_extract(outputPath.c_str(), pathFolder.c_str(), on_extract_entry, &arg);
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Map JSON data extracted from maps.zip.");
}

/// <summary>
/// Load all the Maps from addon storage and store them in the mapInventory.
/// </summary>
void MapLoaderService::loadAllMapsFromStorage() {
	try {
		// Get addon directory
		std::string pathFolder = APIDefs->GetAddonDirectory(ADDON_NAME);
		// Create folder if not exist
		if (!fs::exists(pathFolder)) {
			try {
				fs::create_directory(pathFolder);
			}
			catch (const std::exception& e) {
				std::string message = "Could not create addon directory: ";
				message.append(pathFolder);
				APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, message.c_str());

				// Suppress the warning for the unused variable 'e'
				#pragma warning(suppress: 4101)
				e;
			}
		}

		for (auto lang : SUPPORTED_LOCAL) {
			if (unloading) return;
			// Load events from data.json
			std::string pathData = pathFolder + "/" + lang + ".json";
			if (fs::exists(pathData)) {
				std::ifstream dataFile(pathData);

				if (dataFile.is_open()) {
					json jsonData;
					dataFile >> jsonData;
					dataFile.close();

					gw2::region region = jsonData;

					for (auto map : region.maps)
					{
						if (unloading) return;
						gw2::map* m = new gw2::map(map.second);
						mapInventory->addMap(lang, m);
					}
				}

			}
			else {
				APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, ("Maps file for language not found: " + lang + ".json").c_str());
				continue;
			}
			std::stringstream stream;
			stream << "Maps for locale '" << lang << "' in inventory: " << std::to_string(mapInventory->getLoadedMaps(lang).size());
			APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, stream.str().c_str());
		}
	}

	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Exception in map initialization thread.");
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, e.what());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception in map initialization thread.");
	}
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Map loading from storage complete.");
}

/// <summary>
/// Loads all maps from GW2 API and stores them in the mapInventory.
/// </summary>
void MapLoaderService::loadAllMapsFromApi() {
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Preloading map data from GW2 API.");
	for (auto lang : SUPPORTED_LOCAL) {
		APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, ("Loading maps for locale: " + lang).c_str());
		std::map<std::string, gw2::map> mapInfos = std::map<std::string, gw2::map>();

		auto continentsResponse = performRequest("/v2/continents?lang=" + lang);
		if (continentsResponse.empty()) {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request continents data within retry limits!");
			return;
		}
		json continentsJson = json::parse(continentsResponse);

		for (auto continent : continentsJson.get<std::vector<int>>()) {
			if (unloading) break;
			auto continentResponse = performRequest("/v2/continents/" + std::to_string(continent) + "?lang=" + lang);
			if (continentResponse.empty()) {
				APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request floor data within retry limits!");
				return;
			}

			json continentJson = json::parse(continentResponse);

			for (auto floor : continentJson["floors"].get<std::vector<int>>()) {
				if (unloading) break;
				auto floorResponse = performRequest("/v2/continents/" + std::to_string(continent) + "/floors/" + std::to_string(floor) + "?lang=" + lang);
				if (floorResponse.empty()) {
					APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request region data within retry limits!");
					return;
				}

				json floorJson = json::parse(floorResponse);

				gw2api::continents::floor floor = floorJson.get< gw2api::continents::floor>();
				if (floor.regions.size() == 0) {
					// empty floor, skip
					continue;
				}

				// for every region, for every map create a MapInfo*
				for (auto region: floor.regions)
				{
					for (auto entry: region.second.maps) {
						std::string id = entry.first;
						gw2api::continents::map* map = &entry.second;

						gw2api::continents::map mapInfo;

						if (mapInfos.count(id)) {
							mapInfo = mapInfos[id];
						}
						else {
							mapInfo = *map;
							// enrich with additional data
							mapInfo.continentId = continentJson["id"];
							mapInfo.continentName = continentJson["name"];
							mapInfo.regionId = region.second.id;
							mapInfo.regionName = region.second.name;
							// end enrichment
							mapInfos.emplace(id, mapInfo);
						}

						for (auto sector: map->sectors)
						{
							mapInfo.sectors.emplace(sector);
						}
					}
				}
			}
		}

		// precautionary if we made it here without unloading, store the map data
		if (unloading) return;

		// fill up empty maps with default sectors
		for (auto map: mapInfos)
		{
			// add default sector to maps without sectors
			if (map.second.sectors.size() == 0) {
				gw2api::continents::sector empty = gw2api::continents::sector();
				empty.id = -1;
				empty.name = map.second.name;
				empty.level = 80;
				empty.chatLink = "undefined";
				empty.bounds.push_back({ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() });
				empty.bounds.push_back({ std::numeric_limits<float>::max(), std::numeric_limits<float>::max() });
				map.second.sectors.emplace("-1", empty);
			}
		}

		// add all maps found to the inventory
		for (auto map : mapInfos) {
			gw2::map* m = new gw2::map(map.second);
			mapInventory->addMap(lang, m);
		}

		std::stringstream stream;
		stream << "Maps for locale '" << lang << "' in inventory: " << std::to_string(mapInventory->getLoadedMaps(lang).size());
		APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, stream.str().c_str());
	

	}
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Map loading from API complete.");
}

/*
bool MapLoaderService::loadFromApi(int mapId) {
	// Possible TODO: refactor this to be more efficient;
	// Map loading this way may take 30+ seconds per map, lol

	// Build request URI
	std::string uri = "/v2/maps/";
	uri += std::to_string(mapId);
	uri += "?lang=en"; // TODO can I get the language from Mumble?

	// Log the update call in our debug log, please
	std::string logmsg = "Requesting: " + uri;
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, logmsg.c_str());

	retryCounter = 0;
	httplib::Result res = performRequest(uri);
	if (res == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request map data within retry limits!");
		return false;
	}

	int status = res->status;
	std::string data = res->body;

	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Status: " + std::to_string(status)).c_str());
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Data: " + data).c_str());

	// TODO here we wanna put it into "cool json library" and do as the above
	if (status == 200) {
		nlohmann::json mapJson = nlohmann::json::parse(data);
		std::string name = mapJson["name"].get<std::string>();
		int regionId = mapJson["region_id"].get<int>();
		std::string regionName = mapJson["region_name"].get<std::string>();
		int continentId = mapJson["continent_id"].get<int>();
		std::string continentName = mapJson["continent_name"].get<std::string>();

		MapInfo* mapInfo = new MapInfo(mapId, name, regionId, regionName, continentId, continentName);

		// TODO iterate floors for sectors pew pew
		std::vector<int> floors = mapJson["floors"];
		for (auto floor : floors)
		{
			if (this->unloading) return false; // skip further loading if we are currently unloading the addon
			loadSectors(floor, mapInfo);
		}
		mapInventory->addMap(mapInfo);
		return true;
	}
	else {
		std::stringstream stream;
		stream << "Bad response from server, could not load map data for mapId " << std::to_string(mapId);
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, stream.str().c_str());
	}

	return false;
}

void MapLoaderService::loadSectors(int floor, MapInfo* mapInLoad) {
	// Build request URI
	// v2/continents/{continent_id}/floors/{floor_id}/regions/{region_id}/maps/{map_id}/sectors/{sector_id}

	std::stringstream getAllSectorsUri;
	getAllSectorsUri << "/v2/continents/"
		<< std::to_string(mapInLoad->getContinentId())
		<< "/floors/"
		<< std::to_string(floor)
		<< "/regions/"
		<< std::to_string(mapInLoad->getRegionId())
		<< "/maps/"
		<< std::to_string(mapInLoad->getId())
		<< "/sectors";

	std::string logmsg = "Requesting: " + getAllSectorsUri.str();
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, logmsg.c_str());

	retryCounter = 0;
	httplib::Result res = performRequest(getAllSectorsUri.str());
	if (res == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request sector list within retry limits!");
		return;
	}

	int status = res->status;
	std::string data = res->body;
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Status: " + std::to_string(status)).c_str());
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Data: " + data).c_str());

	if (status == 200) {
		// Ideally we get an array of sector Ids here
		nlohmann::json sectorsJson = nlohmann::json::parse(data);
		std::vector<int> sectorIds = sectorsJson.get<std::vector<int>>();
		for (auto sectorId: sectorIds)
		{
			if (this->unloading) return; // skip further loading if we are currently unloading the addon

			// check if we already know that sector, and if so, skip it
			if(mapInLoad->hasSector(sectorId)) {
				continue;
			}

			loadSector(floor, sectorId, mapInLoad);
		}
	}
	else {
		std::stringstream stream;
		stream << "Bad response from server, could not load sector data for mapId " << std::to_string(mapInLoad->getId());
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, stream.str().c_str());

		// Likely the map has an invalid region, floor or whatever, which could possibly indicate it is a single sector map so add that one
		std::string name = mapInLoad->getName();
		int id = mapInLoad->getId();
		Sector* newSector = new Sector(id, name, -1, "");
		newSector->addBound(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
		newSector->addBound(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		mapInLoad->addSector(newSector);
	}

}
void MapLoaderService::loadSector(int floor, int sectorId, MapInfo* mapInLoad) {

	std::stringstream getSingleSectorUri;
	getSingleSectorUri << "/v2/continents/"
		<< std::to_string(mapInLoad->getContinentId())
		<< "/floors/"
		<< std::to_string(floor)
		<< "/regions/"
		<< std::to_string(mapInLoad->getRegionId())
		<< "/maps/"
		<< std::to_string(mapInLoad->getId())
		<< "/sectors/"
		<< std::to_string(sectorId);
	std::string logmsg = "Requesting: " + getSingleSectorUri.str();
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, logmsg.c_str());

	retryCounter = 0;
	httplib::Result sectorResponse = performRequest(getSingleSectorUri.str());
	if (sectorResponse == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request sector list within retry limits!");
		return;
	}

	int sectorStatus = sectorResponse->status;
	std::string sectorData = sectorResponse->body;
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Status: " + std::to_string(sectorStatus)).c_str());
	APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Data: " + sectorData).c_str());

	if (sectorStatus == 200) {
		nlohmann::json sectorJson = nlohmann::json::parse(sectorData);
		std::string name = sectorJson["name"].get<std::string>();
		int level = sectorJson["level"].get<int>();
		std::string chatLink = sectorJson["chat_link"].get<std::string>();


		Sector* newSector = new Sector(sectorId, name, level, chatLink);

		std::vector<std::vector<float>> bounds = sectorJson["bounds"].get<std::vector<std::vector<float>>>();

		for (auto bound: bounds)
		{
			if (this->unloading) return; // skip further loading if we are currently unloading the addon

			newSector->addBound(bound[0], bound[1]);
		}
		mapInLoad->addSector(newSector);
	}
	else {
		std::stringstream stream;
		stream << "Bad response from server, could not load sector data for mapId " << std::to_string(mapInLoad->getId());
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, stream.str().c_str());
	}
} */
