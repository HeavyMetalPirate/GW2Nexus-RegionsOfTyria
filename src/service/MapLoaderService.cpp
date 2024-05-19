// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "MapLoaderService.h"
#include <Windows.h>
#include "../resource.h"

//#include "../miniz/miniz.h"
#include "../ziplib/src/zip.h"

// HTTPLib Client
const std::string baseUrl = "https://api.guildwars2.com";

int timeoutCounter = 0;
int retryCounter = 0;

// if I put this as a class field I am getting weird "use of deleted function" errors? wtf.
std::thread initializerThread;

namespace fs = std::filesystem;


static int on_extract_entry(const char* filename, void* arg) {
	static int i = 0;
	int n = *(int*)arg;

	APIDefs->Log(ELogLevel::ELogLevel_DEBUG, ADDON_NAME, filename);
	return 0;
}

MapLoaderService::MapLoaderService() {
	this->unloading = false;
}
MapLoaderService::~MapLoaderService() {
	this->unloading = true;
	// Ensure the thread has been started.
	if (initializerThread.joinable()) {
		// This will block until the thread has finished.
		initializerThread.join();
	}
}

httplib::Result MapLoaderService::performRequest(std::string uri) {
	httplib::Client cli(baseUrl);
	auto res = cli.Get(uri);

	while (res == nullptr) {
		if (this->unloading) return res;
		timeoutCounter++;
		retryCounter++;
		// if we retried a lot and always had a timeout, stop doing that now
		if (retryCounter > 10) break;

		res = cli.Get(uri);
	}
	return res;
}

void MapLoaderService::initializeMapStorage() {
	if (!initializerThread.joinable()) {
		// Move a newly constructed thread to the class member.
		initializerThread = std::thread([this]() {
			//this->unpackMaps();
			this->loadAllMapsFromStorage();
		});
	}

}

void MapLoaderService::unpackMaps() {

	// TODO get from internal bundled resource somehow
	HRSRC hResource = FindResource(hSelf, MAKEINTRESOURCE(IDR_MAPS_ZIP), "ZIP");
	if (hResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Did not find resource.");
		return;
	}

	// Hole den Handler f�r die Ressource
	HGLOBAL hLoadedResource = LoadResource(hSelf, hResource);
	if (hLoadedResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Could not load resource.");
		return;
	}

	// Sperre den Speicherbereich der Ressource und erhalte einen Zeiger darauf
	LPVOID lpResourceData = LockResource(hLoadedResource);
	if (lpResourceData == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Could not lo resource.");
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

	// �ffne eine Datei zum Schreiben
	FILE* file = fopen(outputPath.c_str(), "wb"); // "wb" f�r Bin�rschreiben
	if (file == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, "Error trying to write maps.zip (fopen)");
		return;
	}

	// Schreibe die Ressourcendaten in die Datei
	size_t resourceSize = SizeofResource(hSelf, hResource);
	fwrite(lpResourceData, 1, resourceSize, file);

	// Schlie�e die Datei
	fclose(file);
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "maps.zip extracted from module.");
	
	int arg = 2;
	zip_extract(outputPath.c_str(), pathFolder.c_str(), on_extract_entry, &arg);
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, "Map JSON data extracted from maps.zip.");
}


void MapLoaderService::loadAllMapsFromStorage() {
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
		// Load events from data.json
		std::string pathData = pathFolder + "/" + lang + ".json";
		if (fs::exists(pathData)) {
			std::ifstream dataFile(pathData);

			if (dataFile.is_open()) {
				json jsonData;
				dataFile >> jsonData;

				gw2::region region = jsonData;

				for (auto map : region.maps)
				{
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

/*
* Function to update the Map info via GW2 REST API call.
* TODO: refactor to query the following:
* - Map details of map (via v2/maps/{id})
* - all zones of a map (via v2/continents/{continent_id}/floors/{floor_id}/regions/{region_id}/maps/{map_id}/sectors/{sector_id}
*
* how do we get all these?
* - map_id is supplied by mumble identity
* - continent_id and region_id are part of the map details query
* - floor_id comes from an array of ids from the map details query -> iterate over
* - sector_id we get by calling the second url without the param first; it is an array of ids again -> iterate over
*
* what do we do with the data?
* - each sector contains the bounds as an array
* - the plan is to store the sectors for the map in a structured way
* - that way we can later ask "in which zone is the player right now?"
* - if the answer to the above is "it is a different sector than before" we can change a flag indicating that a render for a new zone text is required
*
* tasks to figure out in the future:
* - how to render the zone information for a couple of seconds only? idea: mumble frame counting; how does this compare over different runtimes?
* - do I want to store the map details into JSON data for later usage so I don't need to request it all the time? If doing so I might wanna make the information time out at some point?


void MapLoaderService::loadMap(int mapId) {	
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Loading Map with id: " + std::to_string(mapId)).c_str());

	// Check if the map inventory already knows this map and skip if that's the case
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Checking MapInventory for existence of map...");
	if (mapInventory->getMapInfo(mapId) != nullptr) {
		std::stringstream stream;
		stream << "Requested mapLoad with mapId " << std::to_string(mapId) << " but map is already stored. Skipping mapLoad.";
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, stream.str().c_str());
		return;
	}
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Map not found in inventory, starting to load.");

	timeoutCounter = 0;
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	bool loaded = loadFromStorage(mapId);
	if (!loaded) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Map not found in cache, fetching map from resources.");
		loaded = loadFromResources(mapId);
		if (!loaded) {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Map not found in storage, fetching map from GW2 API.");
			loaded = loadFromApi(mapId);
		}
	}

	if (!loaded) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Couldn't load map.");
		return;
	}
	
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::stringstream stream;
	stream << "Load duration = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]";
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, stream.str().c_str());
}


bool MapLoaderService::loadFromResources(int mapId) {
	std::string lang = "en"; // TODO

	return false;
}

bool MapLoaderService::loadFromStorage(int mapId) {
	std::string lang = "en"; // TODO
	// TODO impl
	// Get addon directory
	std::string pathFolder = APIDefs->GetAddonDirectory("RegionsDisplay");
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

	return false;
}*/

void MapLoaderService::loadAllMapsFromApi() {
	std::map<std::string, gw2::map*> mapInfos = std::map<std::string, gw2::map*>();

	const std::string lang = "en"; // TODO
	auto continentsResponse = performRequest("/v2/continents?lang=" + lang);
	if (continentsResponse == nullptr) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request map data within retry limits!");
		return;
	}
	json continentsJson = json::parse(continentsResponse->body);

	for (auto continent : continentsJson.get<std::vector<int>>()) {
		APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Requesting data from continent " + std::to_string(continent)).c_str());
		auto continentResponse = performRequest("/v2/continents/" + std::to_string(continent) + "?lang=" + lang);
		if (continentResponse == nullptr) {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request map data within retry limits!");
			return;
		}

		json continentJson = json::parse(continentResponse->body);

		for (auto floor : continentJson["floors"].get<std::vector<int>>()) {
			APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Requesting data from floor " + std::to_string(floor)).c_str());
			auto floorResponse = performRequest("/v2/continents/" + std::to_string(continent) + "/floors/" + std::to_string(floor) + "?lang=" + lang);
			if (floorResponse == nullptr) {
				APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not request map data within retry limits!");
				return;
			}

			json floorJson = json::parse(floorResponse->body);

			gw2api::continents::floor floor = floorJson.get< gw2api::continents::floor>();
			if (floor.regions.size() == 0) {
				APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Empty floor detected");
				continue;
			}

			// for every region, for every map create a MapInfo*
			for (auto region : floor.regions)
			{
				for (auto entry : region.second.maps) {
					std::string id = entry.first;
					gw2api::continents::map* map = &entry.second;

					gw2::map* mapInfo;
					if (mapInfos.count(id)) {
						mapInfo = mapInfos[id];
					}
					else {
						mapInfo = new gw2::map(*map);

						APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("New Map: " + map->name).c_str());
						mapInfos.emplace(id, mapInfo);
					}
					for (auto sector : map->sectors)
					{
						mapInfo->sectors.emplace(sector.first, sector.second);
					}			
				}
			}
		}
	}

	// TODO we could possibly store the data in our temp storage here too... just a thought.

	// add all maps found to the inventory
	for (auto map : mapInfos) {
		mapInventory->addMap(lang, map.second);
	}
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
