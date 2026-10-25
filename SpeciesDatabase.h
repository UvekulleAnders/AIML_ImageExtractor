// SpeciesDatabase.h : SQLite database (speciesImages.db) holding species and extracted images
//

#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include "sqlite3.h"

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

	// Stores a PNG-encoded image cut out of a video frame. The image is not
	// assigned to a species (species_id is NULL) until the user classifies it.
	bool AddImage(const CString& sourceFile, long long frameIndex, const cv::Rect& region,
		const std::vector<uchar>& pngData, long long* newId = nullptr);
	long long GetImageCount();

private:
	bool Exec(const char* sql);
	void SetError(const char* context);

	sqlite3* m_db = nullptr;
	CString m_path;
	CString m_lastError;
};
