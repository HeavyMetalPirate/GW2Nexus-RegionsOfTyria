﻿#ifndef SETTINGS_H
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

struct RacialFontSettings {
    std::string race;

    float smallFontSize;
    std::string displayFormatSmall;
    float largeFontSize;
    std::string displayFormatLarge;

    float verticalPosition;
    float spacing;
    float fontScale;
    float fontColor[3];

    std::string widgetDisplayFormat;
    float widgetFontSize;
    float widgetFontColor[3];
};
inline void to_json(json& j, const RacialFontSettings& s) {
    j = json{
        {"race",s.race},
        {"fontSmallSize", s.smallFontSize},
        {"fontLargeSize", s.largeFontSize},
        {"displayFormatSmall", s.displayFormatSmall},
        {"displayFormatLarge", s.displayFormatLarge},
        {"verticalPosition", s.verticalPosition},
        {"spacing", s.spacing},
        {"fontScale", s.fontScale},
        {"fontColor", s.fontColor},
        {"widgetDisplayFormat", s.widgetDisplayFormat},
        {"widgetFontSize", s.widgetFontSize},
        {"widgetFontColor", s.widgetFontColor}
    };
}
inline void from_json(const json& j, RacialFontSettings& s) {
    if (j.contains("race")) {
        j.at("race").get_to(s.race);
    }
    else {
        s.race = "generic";
    }
    if (j.contains("fontSmallSize")) {
        j.at("fontSmallSize").get_to(s.smallFontSize);
    }
    else {
        s.smallFontSize = 26.0f;
    }
    if (j.contains("fontLargeSize")) {
        j.at("fontLargeSize").get_to(s.largeFontSize);
    }
    else {
        s.largeFontSize = 72.0f;
    }
    if (j.contains("displayFormatSmall")) {
        j.at("displayFormatSmall").get_to(s.displayFormatSmall);
    }
    else {
        s.displayFormatSmall = "@c | @r | @m";
    }
    if (j.contains("displayFormatLarge")) {
        j.at("displayFormatLarge").get_to(s.displayFormatLarge);
    }
    else {
        s.displayFormatLarge = "@s";
    }
    if (j.contains("verticalPosition")) {
        j.at("verticalPosition").get_to(s.verticalPosition);
    }
    else {
        s.verticalPosition = 300;
    }
    if (j.contains("spacing")) {
        j.at("spacing").get_to(s.spacing);
    }
    else {
        s.spacing = 25;
    }
    if (j.contains("fontScale")) {
        j.at("fontScale").get_to(s.fontScale);
    }
    else {
        s.fontScale = 1.5f;
    }
    if (j.contains("fontColor")) {
        j.at("fontColor").get_to(s.fontColor);
    }
    else {
        s.fontColor[0] = 255.0f;
        s.fontColor[1] = 255.0f;
        s.fontColor[2] = 255.0f;
    }
    if (j.contains("fontColor")) {
        j.at("fontColor").get_to(s.fontColor);
    }
    else {
        s.fontColor[0] = 255.0f;
        s.fontColor[1] = 255.0f;
        s.fontColor[2] = 255.0f;
    }
    if (j.contains("widgetDisplayFormat")) {
        j.at("widgetDisplayFormat").get_to(s.widgetDisplayFormat);
    }
    else {
        s.widgetDisplayFormat = "@s";
    }
    if (j.contains("widgetFontSize")) {
        j.at("widgetFontSize").get_to(s.widgetFontSize);
    }
    else {
        s.widgetFontSize = 20.0f;
    }
    if (j.contains("widgetFontColor")) {
        j.at("widgetFontColor").get_to(s.widgetFontColor);
    }
    else {
        s.widgetFontColor[0] = 255.0f;
        s.widgetFontColor[1] = 255.0f;
        s.widgetFontColor[2] = 255.0f;
    }
}

/// <summary>
/// Settings Struct & JSON functionality
/// </summary>
struct Settings {
    int fontsVersion;

    Locale locale;
    
    // 0 = generic, 1 = asura, 2 = charr, 3 = human, 4 = norn, 5 = sylvari
    RacialFontSettings fontSettings[6];

    // required for WvW maps
    int worldId;

    // TODO settings for font overrides / modes i.e. "use racial fonts", "use just font of race X or generic font" etc.
    int fontMode;
    bool disableAnimations;
    bool enablePopup;
    bool hidePopupInCompetitive;
    bool hidePopupInCombat;

    // Widget generic stuff
    bool widgetEnabled;
    float widgetPositionX;
    float widgetPositionY;
    float widgetWidth;
    float widgetBackgroundOpacity;
    int widgetTextAlign;

    // legacy display settings, deprecated
    std::string displayFormatSmall;
    std::string displayFormatLarge;
    float verticalPosition;
    float spacing;
    float fontScale;
    float fontColor[3];

};
inline void to_json(json& j, const Settings& settings) {
    j = json{
        {"fontsVersion", settings.fontsVersion},
        {"locale", settings.locale},
        {"worldId", settings.worldId},
        {"fontMode", settings.fontMode},
        {"enablePopup", settings.enablePopup},
        {"hidePopupInCompetitive", settings.hidePopupInCompetitive},
        {"hidePopupInCombat", settings.hidePopupInCombat},
        {"disableAnimations", settings.disableAnimations},
        {"fontSettings", settings.fontSettings},
        {"widgetEnabled", settings.widgetEnabled},
        {"widgetPositionX", settings.widgetPositionX},
        {"widgetPositionY", settings.widgetPositionY},
        {"widgetWidth", settings.widgetWidth},
        {"widgetBackgroundOpacity", settings.widgetBackgroundOpacity},
        {"widgetTextAlign", settings.widgetTextAlign}
    };
}
inline void from_json(const json& j, Settings& settings) {
    if (j.contains("fontsVersion")) {
        j.at("fontsVersion").get_to(settings.fontsVersion);
    }
    else {
        settings.fontsVersion = 0;
    }
    if (j.contains("locale")) {
        j.at("locale").get_to(settings.locale);
    }
    else {
        settings.locale = Locale::En; // Default
    }
    if (j.contains("enablePopup")) {
        j.at("enablePopup").get_to(settings.enablePopup);
    }
    else {
        settings.enablePopup = true;
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
    if (j.contains("worldId")) {
        j.at("worldId").get_to(settings.worldId);
    }
    else {
        settings.worldId = 0;
    }
    if (j.contains("fontMode")) {
        j.at("fontMode").get_to(settings.fontMode);
    }
    else {
        settings.fontMode = 0;
    }
    if (j.contains("disableAnimations")) {
        j.at("disableAnimations").get_to(settings.disableAnimations);
    }
    else {
        settings.disableAnimations = false;
    }
    if (j.contains("hidePopupInCompetitive")) {
        j.at("hidePopupInCompetitive").get_to(settings.hidePopupInCompetitive);
    }
    else {
        settings.hidePopupInCompetitive = false;
    }
    if (j.contains("hidePopupInCombat")) {
        j.at("hidePopupInCombat").get_to(settings.hidePopupInCombat);
    }
    else {
        settings.hidePopupInCombat = false;
    }
    if (j.contains("fontSettings")) {
        j.at("fontSettings").get_to(settings.fontSettings);
    }
    else {
        // init everything with the current generic
        for (int i = 0; i < 6; i++) {
            RacialFontSettings s = RacialFontSettings();
            s.race = i == 0 ? "Generic" :
                i == 1 ? "Asura" :
                i == 2 ? "Charr" :
                i == 3 ? "Human" :
                i == 4 ? "Norn" :
                i == 5 ? "Sylvari" : "wtf";
            s.displayFormatSmall = settings.displayFormatSmall;
            s.displayFormatLarge = settings.displayFormatLarge;
            s.spacing = settings.spacing;
            s.verticalPosition = settings.verticalPosition;
            s.fontScale = settings.fontScale;
            s.fontColor[0] = settings.fontColor[0];
            s.fontColor[1] = settings.fontColor[1];
            s.fontColor[2] = settings.fontColor[2];
            s.largeFontSize = 72.0f;
            s.smallFontSize = 28.0f;

            settings.fontSettings[i] = s;
        }
    }
    if (j.contains("widgetEnabled")) {
        j.at("widgetEnabled").get_to(settings.widgetEnabled);
    }
    else {
        settings.widgetEnabled = false;
    }
    if (j.contains("widgetPositionX")) {
        j.at("widgetPositionX").get_to(settings.widgetPositionX);
    }
    else {
        settings.widgetPositionX = 100.0f;
    }
    if (j.contains("widgetPositionY")) {
        j.at("widgetPositionY").get_to(settings.widgetPositionY);
    }
    else {
        settings.widgetPositionY = 100.0f;
    }
    if (j.contains("widgetWidth")) {
        j.at("widgetWidth").get_to(settings.widgetWidth);
    }
    else {
        settings.widgetWidth = 200.0f;
    }
    if (j.contains("widgetBackgroundOpacity")) {
        j.at("widgetBackgroundOpacity").get_to(settings.widgetBackgroundOpacity);
    }
    else {
        settings.widgetBackgroundOpacity = 0.8f;
    }
    if (j.contains("widgetTextAlign")) {
        j.at("widgetTextAlign").get_to(settings.widgetTextAlign);
    }
    else {
        settings.widgetTextAlign = 0;
    }
}
/// ================================================================================


// Non stored settings
extern bool showDebug;
extern bool showTemplate[6];
extern int templateRace;

// Stored settings
extern Settings settings;

#endif