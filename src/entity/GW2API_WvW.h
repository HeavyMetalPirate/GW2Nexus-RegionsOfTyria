#ifndef GW2API_WVW_H
#define GW2API_WVW_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// <summary>
/// Contains structs and json converters for the relevant GW2API endpoints
/// </summary>
namespace gw2api::wvw {
	
	// Helpers for date parsing
	inline std::chrono::system_clock::time_point parse_date(const std::string& date_str) {
		std::tm tm = {};
		std::istringstream ss(date_str);
		ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
		return std::chrono::system_clock::from_time_t(std::mktime(&tm));
	}
	inline std::string format_date(const std::chrono::system_clock::time_point& time_point) {
		std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
		std::tm tm;
		gmtime_s(&tm, &time_t);
		std::ostringstream ss;
		ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
		return ss.str();
	}

	struct worlds {
		int red;
		int blue;
		int green;
	};
	inline void to_json(json& j, const worlds& w) {
		j = json{ 
			{"red", w.red}, 
			{"blue", w.blue}, 
			{"green", w.green} 
		};
	}

	inline void from_json(const json& j, worlds& w) {
		j.at("red").get_to(w.red);
		j.at("blue").get_to(w.blue);
		j.at("green").get_to(w.green);
	}

	struct match {
		std::string id;
		std::chrono::system_clock::time_point start_time;
		std::chrono::system_clock::time_point end_time;
		worlds worlds;
	};
	inline void to_json(json& j, const match& match) {
		j = json{
			{"id", match.id},
			{"start_time", format_date(match.start_time)}, 
			{"end_time", format_date(match.end_time)}, 
			{"worlds", match.worlds}
		};
	}
	inline void from_json(const json& j, match& match) {
		j.at("id").get_to(match.id);
		match.start_time = parse_date(j.at("start_time").get<std::string>());
		match.end_time = parse_date(j.at("end_time").get<std::string>());
		//j.at("worlds").get_to(match.worlds);

		json allworlds = j.at("all_worlds");
		worlds worlds = {
			allworlds["red"][0],
			allworlds["blue"][0],
			allworlds["green"][0]
		};
		match.worlds = worlds;
	}
}

#endif