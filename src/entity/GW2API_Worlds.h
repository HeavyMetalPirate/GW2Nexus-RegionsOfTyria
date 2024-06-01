#ifndef GW2API_WORLDS_H
#define GW2API_WORLDS_H

#include <string>
#include <vector>
#include <map>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// <summary>
/// Contains structs and json converters for the relevant GW2API endpoints
/// </summary>
namespace gw2api::worlds {
	struct world {
		int id;
		std::string name;
		std::string population;
	};
	inline void to_json(json& j, const world& world) {
		j = json{
			{"id", world.id},
			{"name", world.name},
			{"population", world.population}
		};
	}
	inline void from_json(const json& j, world& world) {
		j.at("id").get_to(world.id);
		j.at("name").get_to(world.name);
		j.at("population").get_to(world.population);
	}
}

#endif