#ifndef GW2API_CONTINENTS_H
#define GW2API_CONTINENTS_H

#include <string>
#include <vector>
#include <map>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// <summary>
/// Contains structs and json converters for the relevant GW2API endpoints
/// </summary>
namespace gw2api::continents {

	/// <summary>
	/// Struct for a coordinate representation
	/// </summary>
	struct coordinate {
		float x;
		float y;
	};
	inline void to_json(json& j, const coordinate& coordinate) {
		j.push_back(coordinate.x);
		j.push_back(coordinate.y);
	}
	inline void from_json(const json& j, coordinate& coord) {
		coord.x = j[0];
		coord.y = j[1];
	}
	///=========================================================

	/// <summary>
	/// Struct for a map sector representation
	/// </summary>
	struct sector {
		int id;
		int level;
		std::string name;
		std::string chatLink;
		std::vector<coordinate> bounds;
	};
	inline void to_json(json& j, const sector& sector) {
		j = json{
			{"id", sector.id},
			{"level", sector.level},
			{"name", sector.name},
			{"chat_link", sector.chatLink},
			{"bounds", sector.bounds}
		};
	}
	inline void from_json(const json& j, sector& sector) {
		j.at("id").get_to(sector.id);
		if (j.contains("level"))
			j.at("level").get_to(sector.level);
		if (j.contains("name"))
			j.at("name").get_to(sector.name);
		if (j.contains("chat_link")) {
			j.at("chat_link").get_to(sector.chatLink);
		}
		j.at("bounds").get_to(sector.bounds);
	}
	///=========================================================

	/* For later usage */
	struct poi {
		int id;
	};
	struct task {
		int id;
	};
	struct skillchallenge {
		int id;
	};
	///=========================================================

	/// <summary>
	/// struct for map representation
	/// </summary>
	struct map {
		int id;
		int minLevel;
		int maxLevel;
		std::string name;
		// additional non-standard data
		int regionId;
		std::string regionName;
		int continentId;
		std::string continentName;
		// end additional data
		std::map<std::string, sector> sectors;
		//std::map<std::string, poi> pois;
		//std::map<std::string, task> tasks;
		//std::map<std::string, skillchallenge> skillChallenges;
	};
	inline void to_json(json& j, const map& map) {
		j = json{
			{"id", map.id},
			{"min_level", map.minLevel},
			{"max_level", map.maxLevel},
			{"name", map.name},
			// additional non-standard data
			{"regionId", map.regionId},
			{"regionName", map.regionName},
			{"continentId", map.continentId},
			{"continentName", map.continentName},
			// end additional data
			{"sectors", map.sectors}
		};
	}
	inline void from_json(const json& j, map& map) {
		j.at("id").get_to(map.id);
		if (j.contains("name"))
			j.at("name").get_to(map.name);
		if (j.contains("min_level"))
			j.at("min_level").get_to(map.minLevel);
		if (j.contains("max_level"))
			j.at("max_level").get_to(map.maxLevel);
		j.at("sectors").get_to(map.sectors);

		// additional non-standard data
		if (j.contains("regionId"))
			j.at("regionId").get_to(map.regionId);
		if (j.contains("regionName"))
			j.at("regionName").get_to(map.regionName);
		if (j.contains("continentId"))
			j.at("continentId").get_to(map.continentId);
		if (j.contains("continentName"))
			j.at("continentName").get_to(map.continentName);
		// end additional data
	}
	///=========================================================

	/// <summary>
	/// struct for region representation
	/// </summary>
	struct region {
		int id;
		std::string name;
		std::map<std::string, map> maps;
	};
	inline void to_json(json& j, const region& region) {
		j = json{
			{"id", region.id},
			{"name", region.name},
			{"maps", region.maps}
		};
	}
	inline void from_json(const json& j, region& region) {
		j.at("id").get_to(region.id);
		if (j.contains("name"))
			j.at("name").get_to(region.name);
		j.at("maps").get_to(region.maps);
	}
	///=========================================================

	/// <summary>
	/// struct for floor representation
	/// </summary>
	struct floor {
		int id;
		std::map<std::string, region> regions;
	};
	inline void to_json(json& j, const floor& floor) {
		j = json{
			{"id", floor.id},
			{"regions", floor.regions}
		};
	}
	inline void from_json(const json& j, floor& floor) {
		j.at("id").get_to(floor.id);
		j.at("regions").get_to(floor.regions);
	}
	///=========================================================

	/// <summary>
	/// struct for continent representation
	/// </summary>
	struct continent {
		int id;
		std::string name;
		std::vector<floor> floors;
	};
}

#endif