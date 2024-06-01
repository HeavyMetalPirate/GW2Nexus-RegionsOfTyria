#ifndef WORLD_INVENTORY_H
#define WORLD_INVENTORY_H

#include <map>
#include "../entity/GW2API_Worlds.h"



class WorldInventory {
public:
	WorldInventory();

	void addWorld(std::string locale, gw2api::worlds::world* world);
	gw2api::worlds::world* getWorld(std::string locale, int id);
	std::vector<gw2api::worlds::world*> getAllWorlds(std::string locale);

private:
	/// <summary>
	/// Map with the loaded maps per locale
	/// outer map: locale -> map
	/// inner map: worldId -> world data
	/// </summary>
	std::map<std::string, std::map<int, gw2api::worlds::world*>> loadedWorlds = std::map<std::string, std::map<int, gw2api::worlds::world*>>();
};

#endif /* WORLD_INVENTORY_H */
