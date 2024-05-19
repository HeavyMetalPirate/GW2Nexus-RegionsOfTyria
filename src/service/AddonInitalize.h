#ifndef INITIALIZE_SERVICE_H
#define INITIALIZE_SERVICE_H

#include "../Globals.h"
#include "../resource.h"
#include "../ziplib/src/zip.h"

#include <Windows.h>

namespace fs = std::filesystem;

static int on_extract_entry(const char* filename, void* arg) {
	static int i = 0;
	int n = *(int*)arg;

	APIDefs->Log(ELogLevel::ELogLevel_DEBUG, ADDON_NAME, filename);
	return 0;
}

static void unpackResource(const int resourceName, const std::string& resourceType, const std::string& targetFileName, bool overwrite = true) {

	// TODO get from internal bundled resource somehow
	//HRSRC hResource = FindResource(hSelf, MAKEINTRESOURCE(IDR_MAPS_ZIP), "ZIP");
	HRSRC hResource = FindResource(hSelf, MAKEINTRESOURCE(resourceName), resourceType.c_str());
	if (hResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, ("Did not find resource: " + targetFileName).c_str());
		return;
	}

	// Hole den Handler f�r die Ressource
	HGLOBAL hLoadedResource = LoadResource(hSelf, hResource);
	if (hLoadedResource == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, ("Could not load resource: " + targetFileName).c_str());
		return;
	}

	// Sperre den Speicherbereich der Ressource und erhalte einen Zeiger darauf
	LPVOID lpResourceData = LockResource(hLoadedResource);
	if (lpResourceData == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, ("Could not lock resource: " + targetFileName).c_str());
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
	std::string outputPath = pathFolder + "/" + targetFileName;

	if (fs::exists(outputPath) && !overwrite) {
		APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, ("Resource already exists, and should not be overwritten: " + targetFileName).c_str());
		return;
	}

	// �ffne eine Datei zum Schreiben
	FILE* file = fopen(outputPath.c_str(), "wb"); // "wb" f�r Bin�rschreiben
	if (file == NULL) {
		APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, ("Error trying to write (fopen): " + targetFileName).c_str());
		return;
	}

	// Schreibe die Ressourcendaten in die Datei
	size_t resourceSize = SizeofResource(hSelf, hResource);
	fwrite(lpResourceData, 1, resourceSize, file);

	// Schlie�e die Datei
	fclose(file);
	APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, (targetFileName + " extracted from module.").c_str());

	if ("ZIP" == resourceType) {
		int arg = 2;
		zip_extract(outputPath.c_str(), pathFolder.c_str(), on_extract_entry, &arg);
		APIDefs->Log(ELogLevel::ELogLevel_INFO, ADDON_NAME, ("Extracted data from " + targetFileName).c_str());
	}
}

static void unpackResources() {
	unpackResource(IDR_MAPS_ZIP, "ZIP", "Maps.zip");
	unpackResource(IDR_FONTS_CHARR, "TTF", "font_charr.ttf", false);
	unpackResource(IDR_FONTS_HUMAN, "TTF", "font_human.ttf", false);
	unpackResource(IDR_FONTS_SYLVARI, "TTF", "font_sylvari.ttf", false);
	unpackResource(IDR_FONTS_NORN, "TTF", "font_norn.ttf", false);
	unpackResource(IDR_FONTS_ASURA, "TTF", "font_asura.ttf", false);
}


#endif
