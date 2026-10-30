// SpeciesDatabase.h : SQLite database (speciesImages.db) holding species and extracted images
//

#pragma once

#include "AgeCodes.h"
#include <vector>
#include <opencv2/core.hpp>
#include "sqlite3.h"

// A bird species. Swedish name is the unique key shown to the user.
struct SpeciesRecord
{
	long long id = 0;
	CString swedishName;
	CString latinName;
	CString englishName;

	// The name to show: Swedish if present, otherwise English, otherwise Latin (never empty for stored species).
	CString DisplayName() const
	{
		return !swedishName.IsEmpty() ? swedishName : !englishName.IsEmpty() ? englishName : latinName;
	}
};

// Id of the default 'Unknown' species that is always present.
constexpr long long kUnknownSpeciesId = 1;

// An extracted image without its PNG data.
struct ImageRecord
{
	long long id = 0;
	long long speciesId = 0;
	CString sourceFile;
	long long frameIndex = 0;
	int x = 0, y = 0, width = 0, height = 0;
	CString ageCode = _T("U"); // USGS BBL year-class code, see AgeCodes.h
};

class CSpeciesDatabase
{
public:
	CSpeciesDatabase() = default;
	~CSpeciesDatabase() { Close(); }
	CSpeciesDatabase(const CSpeciesDatabase&) = delete;
	CSpeciesDatabase& operator=(const CSpeciesDatabase&) = delete;

	// Opens the database file, creating it and its tables if they do not exist.
	bool Open(const CString& path);
	void Close();
	bool IsOpen() const { return m_db != nullptr; }
	const CString& GetPath() const { return m_path; }
	const CString& GetLastError() const { return m_lastError; }

	// Stores a PNG-encoded image cut out of a video frame, associated with a species.
	bool AddImage(const CString& sourceFile, long long frameIndex, const cv::Rect& region,
		const std::vector<uchar>& pngData, long long speciesId = kUnknownSpeciesId, long long* newId = nullptr);

	// All species ordered by Swedish name, with 'Unknown' first.
	std::vector<SpeciesRecord> GetSpecies();
	// Adds a species. The Latin name is compulsory and unique (ignoring case); the Swedish and English names may be empty.
	bool AddSpecies(const SpeciesRecord& species, long long* newId = nullptr);
	// Replaces the names of a species (the id in 'species' selects it). Fails for the Unknown species,
	// an unknown id, an empty Latin name, or a Latin name used by another species.
	bool UpdateSpecies(const SpeciesRecord& species);
	// Sets the age (year-class) code of an image; fails for codes not listed in AgeCodes.h.
	bool SetImageAge(long long imageId, const CString& ageCode);
	// Links an image to another existing species.
	bool SetImageSpecies(long long imageId, long long speciesId);
	long long GetImageCount();
	// All stored images (metadata only), ordered by species and id.
	std::vector<ImageRecord> GetImages();
	// Reads the PNG data of one image.
	bool GetImagePng(long long imageId, std::vector<uchar>& png);

private:
	bool Exec(const char* sql);
	bool MigrateSpeciesTable();
	bool LatinNameInUse(const CString& latinName, long long exceptId);
	bool ColumnExists(const char* table, const char* column);
	void SetError(const char* context);

	sqlite3* m_db = nullptr;
	CString m_path;
	CString m_lastError;
};
