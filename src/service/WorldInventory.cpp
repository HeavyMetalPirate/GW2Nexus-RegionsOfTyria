#include "WorldInventory.h"

WorldInventory::WorldInventory() {

}

void WorldInventory::addWorld(std::string locale, gw2api::worlds::world* world) {
	this->loadedWorlds[locale][world->id] = world;
}
void WorldInventory::addAlliance(std::string locale, gw2api::worlds::alliance* alliance) {
	this->loadedAlliances[locale][alliance->id] = alliance;
}
gw2api::worlds::world* WorldInventory::getWorld(std::string locale, int id) {
	if (this->loadedWorlds[locale].count(id))
		return this->loadedWorlds[locale][id];
	else
		return nullptr;
}
gw2api::worlds::alliance* WorldInventory::getAlliance(std::string locale, int id) {
	if (this->loadedAlliances[locale].count(id))
		return this->loadedAlliances[locale][id];
	else
		return nullptr;
}

std::vector<gw2api::worlds::world*> WorldInventory::getAllWorlds(std::string locale) {
	std::vector < gw2api::worlds::world*> worlds = std::vector<gw2api::worlds::world*>();

	for (auto w : this->loadedWorlds[locale]) {
		worlds.push_back(w.second);
	}

	std::sort(worlds.begin(), worlds.end(), [](const auto& a, const auto& b) {
		return a->name < b->name;
	});

	return worlds;
}


std::vector<gw2api::worlds::alliance*> WorldInventory::getAllAlliances(std::string locale) {
	std::vector < gw2api::worlds::alliance*> alliances = std::vector<gw2api::worlds::alliance*>();

	for (auto w : this->loadedAlliances[locale]) {
		alliances.push_back(w.second);
	}

	std::sort(alliances.begin(), alliances.end(), [](const auto& a, const auto& b) {
		return a->name < b->name;
		});

	return alliances;
}