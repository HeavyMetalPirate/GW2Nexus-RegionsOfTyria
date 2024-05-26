#ifndef SETTINGS_H
#define SETTINGS_H

#include <nlohmann/json.hpp>
using json = nlohmann::json;

enum class Locale {
	//Client,
	En,
	De,
	Es,
	Fr
	//Zh << in theory we have the data, but in reality it isn't being rendered properly at all
};
inline void to_json(json& j, const Locale& locale) {
    switch (locale) {
    case Locale::En: j = "en"; break;
    case Locale::De: j = "de"; break;
    case Locale::Es: j = "es"; break;
    case Locale::Fr: j = "fr"; break;
    //case Locale::Zh: j = "zh"; break;

    default: throw std::invalid_argument("Invalid color value");
    }
}
inline void from_json(const json& j, Locale& locale) {
    std::string colorStr = j.get<std::string>();
    if (colorStr == "en") {
        locale = Locale::En;
    }
    else if (colorStr == "de") {
        locale = Locale::De;
    }
    else if (colorStr == "es") {
        locale = Locale::Es;
    }
    else if (colorStr == "fr") {
        locale = Locale::Fr;
    }
    //else if (colorStr == "zh") {
    //    locale = Locale::Zh;
    //}
    else {
        throw std::invalid_argument("Invalid locale value");
    }
}

inline const std::string GetLocaleAsString(Locale value) {
    switch (value)
    {
    //case Locale::Client: return "en"; // TODO read locale from client configuration instead
    case Locale::En: return "en";
    case Locale::De: return "de";
    case Locale::Es: return "es";
    case Locale::Fr: return "fr";
    //case Locale::Zh: return "zh";
    default: return "Unknown";
    }
}
struct LocaleItem {
    Locale value;
    std::string name;
    std::string description;

    LocaleItem(Locale v, const std::string& n, const std::string& d)
        : value(v), name(n), description(d) {}
};
inline std::vector<LocaleItem> localeItems = {
    //{ Locale::Client, "client", "Same as client" },
    { Locale::En, "en", "English" },
    { Locale::De, "de", "Deutsch" },
    { Locale::Es, "es", "Español" },
    { Locale::Fr, "fr", "Français" }
    //{ Locale::Zh, "zh", "中文" }
};

/// ================================================================================
/// <summary>
/// Settings Struct & JSON functionality
/// </summary>
struct Settings {
    Locale locale;
    
    std::string displayFormatSmall;
    std::string displayFormatLarge;

    float verticalPosition;
    float spacing;
    float fontScale;
    float fontColor[3];
};
inline void to_json(json& j, const Settings& settings) {
    j = json{
        {"locale", settings.locale},
        {"displayFormatSmall", settings.displayFormatSmall},
        {"displayFormatLarge", settings.displayFormatLarge},
        {"verticalPosition", settings.verticalPosition},
        {"spacing", settings.spacing},
        {"fontScale", settings.fontScale},
        {"fontColor", settings.fontColor}
    };
}
inline void from_json(const json& j, Settings& settings) {
    if (j.contains("locale")) {
        j.at("locale").get_to(settings.locale);
    }
    else {
        settings.locale = Locale::En; // Default
    }
    if (j.contains("displayFormatSmall")) {
        j.at("displayFormatSmall").get_to(settings.displayFormatSmall);
    }
    else {
        settings.displayFormatSmall = "@c | @r | @m";
    }
    if (j.contains("displayFormatLarge")) {
        j.at("displayFormatLarge").get_to(settings.displayFormatLarge);
    }
    else {
        settings.displayFormatLarge = "@s";
    }
    if (j.contains("verticalPosition")) {
        j.at("verticalPosition").get_to(settings.verticalPosition);
    }
    else {
        settings.verticalPosition = 300;
    }
    if (j.contains("spacing")) {
        j.at("spacing").get_to(settings.spacing);
    }
    else {
        settings.spacing = 25;
    }
    if (j.contains("fontScale")) {
        j.at("fontScale").get_to(settings.fontScale);
    }
    else {
        settings.fontScale = 1.5f;
    }
    if (j.contains("fontColor")) {
        j.at("fontColor").get_to(settings.fontColor);
    }
    else {
        settings.fontColor[0] = 255.0f;
        settings.fontColor[1] = 255.0f;
        settings.fontColor[2] = 255.0f;
    }
}
/// ================================================================================


// Non stored settings
extern bool showDebug;
extern bool showTemplate;

// Stored settings
extern Settings settings;

#endif