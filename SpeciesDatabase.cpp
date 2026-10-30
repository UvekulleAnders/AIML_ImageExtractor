// SpeciesDatabase.cpp : implementation of the CSpeciesDatabase class
//

#include "pch.h"
#include "SpeciesDatabase.h"
#include <string>

namespace
{
	const char* const kSchema =
		"PRAGMA foreign_keys = ON;"
		"CREATE TABLE IF NOT EXISTS species ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  swedish_name TEXT NOT NULL DEFAULT '',"
		"  latin_name TEXT,"
		"  english_name TEXT,"
		"  created_at TEXT NOT NULL DEFAULT (datetime('now'))"
		");"
		"CREATE TABLE IF NOT EXISTS images ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  species_id INTEGER REFERENCES species(id) ON DELETE SET NULL,"
		"  source_file TEXT NOT NULL,"
		"  frame_index INTEGER NOT NULL,"
		"  x INTEGER NOT NULL,"
		"  y INTEGER NOT NULL,"
		"  width INTEGER NOT NULL,"
		"  height INTEGER NOT NULL,"
		"  image_png BLOB NOT NULL,"
		"  age_code TEXT NOT NULL DEFAULT 'U',"
		"  created_at TEXT NOT NULL DEFAULT (datetime('now'))"
		");"
		"CREATE INDEX IF NOT EXISTS idx_images_species ON images(species_id);";
}

bool CSpeciesDatabase::Open(const CString& path)
{
	Close();
	m_path = path;

	if (sqlite3_open16(path.GetString(), &m_db) != SQLITE_OK)
	{
		SetError("open");
		Close();
		return false;
	}
	// Databases created by the first version had species.name and no english_name.
	if (ColumnExists("species", "name") && !ColumnExists("species", "swedish_name") &&
		!Exec("ALTER TABLE species RENAME COLUMN name TO swedish_name;"))
	{
		Close();
		return false;
	}
	if (!Exec(kSchema) || !MigrateSpeciesTable() ||
		(!ColumnExists("images", "age_code") && !Exec("ALTER TABLE images ADD COLUMN age_code TEXT NOT NULL DEFAULT 'U';")) ||
		(ColumnExists("species", "swedish_name") && !ColumnExists("species", "english_name") &&
			!Exec("ALTER TABLE species ADD COLUMN english_name TEXT;")) ||
		!Exec("INSERT OR IGNORE INTO species (id, swedish_name, latin_name, english_name) VALUES (1, 'Unknown', '', 'Unknown');"))
	{
		Close();
		return false;
	}
	return true;
}

// Older databases declared swedish_name UNIQUE, which prevents several species without a Swedish name.
// The Latin name identifies a species instead, so the table is rebuilt without that constraint.
bool CSpeciesDatabase::MigrateSpeciesTable()
{
	sqlite3_stmt* stmt = nullptr;
	bool needsRebuild = false;
	if (sqlite3_prepare_v2(m_db, "SELECT sql FROM sqlite_master WHERE type = 'table' AND name = 'species';", -1, &stmt, nullptr) == SQLITE_OK &&
		sqlite3_step(stmt) == SQLITE_ROW)
	{
		const char* sql = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		needsRebuild = sql != nullptr && strstr(sql, "UNIQUE") != nullptr;
	}
	sqlite3_finalize(stmt);
	if (!needsRebuild)
		return true;

	if (!ColumnExists("species", "english_name") && !Exec("ALTER TABLE species ADD COLUMN english_name TEXT;"))
		return false;
	// Dropping the parent table with foreign keys on would null out images.species_id.
	if (!Exec("PRAGMA foreign_keys = OFF;") ||
		!Exec("BEGIN;"
			"CREATE TABLE species_new ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  swedish_name TEXT NOT NULL DEFAULT '',"
			"  latin_name TEXT,"
			"  english_name TEXT,"
			"  created_at TEXT NOT NULL DEFAULT (datetime('now'))"
			");"
			"INSERT INTO species_new (id, swedish_name, latin_name, english_name, created_at)"
			" SELECT id, swedish_name, latin_name, english_name, created_at FROM species;"
			"DROP TABLE species;"
			"ALTER TABLE species_new RENAME TO species;"
			"COMMIT;"))
	{
		Exec("ROLLBACK;");
		Exec("PRAGMA foreign_keys = ON;");
		return false;
	}
	return Exec("PRAGMA foreign_keys = ON;");
}

void CSpeciesDatabase::Close()
{
	if (m_db != nullptr)
	{
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

bool CSpeciesDatabase::AddImage(const CString& sourceFile, long long frameIndex, const cv::Rect& region,
	const std::vector<uchar>& pngData, long long speciesId, long long* newId)
{
	if (m_db == nullptr || pngData.empty())
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql =
		"INSERT INTO images (source_file, frame_index, x, y, width, height, image_png, species_id)"
		" VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}

	sqlite3_bind_text16(stmt, 1, sourceFile.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, frameIndex);
	sqlite3_bind_int(stmt, 3, region.x);
	sqlite3_bind_int(stmt, 4, region.y);
	sqlite3_bind_int(stmt, 5, region.width);
	sqlite3_bind_int(stmt, 6, region.height);
	sqlite3_bind_blob64(stmt, 7, pngData.data(), pngData.size(), SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 8, speciesId);

	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("insert");
	else if (newId != nullptr)
		*newId = sqlite3_last_insert_rowid(m_db);
	sqlite3_finalize(stmt);
	return ok;
}

std::vector<SpeciesRecord> CSpeciesDatabase::GetSpecies()
{
	std::vector<SpeciesRecord> species;
	if (m_db == nullptr)
		return species;

	sqlite3_stmt* stmt = nullptr;
	const char* sql =
		"SELECT id, swedish_name, COALESCE(latin_name, ''), COALESCE(english_name, '') FROM species"
		" ORDER BY (id = 1) DESC, swedish_name COLLATE NOCASE;";
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return species;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		SpeciesRecord record;
		record.id = sqlite3_column_int64(stmt, 0);
		record.swedishName = reinterpret_cast<LPCWSTR>(sqlite3_column_text16(stmt, 1));
		record.latinName = reinterpret_cast<LPCWSTR>(sqlite3_column_text16(stmt, 2));
		record.englishName = reinterpret_cast<LPCWSTR>(sqlite3_column_text16(stmt, 3));
		species.push_back(record);
	}
	sqlite3_finalize(stmt);
	return species;
}

bool CSpeciesDatabase::LatinNameInUse(const CString& latinName, long long exceptId)
{
	CString wanted(latinName);
	wanted.Trim();
	for (const SpeciesRecord& existing : GetSpecies())
	{
		if (existing.id != exceptId && !existing.latinName.IsEmpty() && existing.latinName.CompareNoCase(wanted) == 0)
			return true;
	}
	return false;
}

bool CSpeciesDatabase::AddSpecies(const SpeciesRecord& species, long long* newId)
{
	if (m_db == nullptr || species.latinName.IsEmpty() || LatinNameInUse(species.latinName, 0))
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = "INSERT INTO species (swedish_name, latin_name, english_name) VALUES (?1, ?2, ?3);";
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}
	sqlite3_bind_text16(stmt, 1, species.swedishName.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text16(stmt, 2, species.latinName.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text16(stmt, 3, species.englishName.GetString(), -1, SQLITE_TRANSIENT);

	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("insert species");
	else if (newId != nullptr)
		*newId = sqlite3_last_insert_rowid(m_db);
	sqlite3_finalize(stmt);
	return ok;
}

bool CSpeciesDatabase::UpdateSpecies(const SpeciesRecord& species)
{
	if (m_db == nullptr || species.id == kUnknownSpeciesId || species.latinName.IsEmpty() ||
		LatinNameInUse(species.latinName, species.id))
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = "UPDATE species SET swedish_name = ?1, latin_name = ?2, english_name = ?3 WHERE id = ?4;";
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}
	sqlite3_bind_text16(stmt, 1, species.swedishName.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text16(stmt, 2, species.latinName.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text16(stmt, 3, species.englishName.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 4, species.id);

	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("update species");
	else
		ok = sqlite3_changes(m_db) == 1;
	sqlite3_finalize(stmt);
	return ok;
}

bool CSpeciesDatabase::SetImageAge(long long imageId, const CString& ageCode)
{
	if (m_db == nullptr || !IsValidAgeCode(ageCode))
		return false;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "UPDATE images SET age_code = ?1 WHERE id = ?2;", -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}
	sqlite3_bind_text16(stmt, 1, ageCode.GetString(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, imageId);

	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("update image age");
	else
		ok = sqlite3_changes(m_db) == 1;
	sqlite3_finalize(stmt);
	return ok;
}

bool CSpeciesDatabase::SetImageSpecies(long long imageId, long long speciesId)
{
	if (m_db == nullptr)
		return false;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "UPDATE images SET species_id = ?1 WHERE id = ?2;", -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}
	sqlite3_bind_int64(stmt, 1, speciesId);
	sqlite3_bind_int64(stmt, 2, imageId);

	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("update image");
	else
		ok = sqlite3_changes(m_db) == 1;
	sqlite3_finalize(stmt);
	return ok;
}

bool CSpeciesDatabase::ColumnExists(const char* table, const char* column)
{
	sqlite3_stmt* stmt = nullptr;
	const std::string sql = std::string("PRAGMA table_info(") + table + ");";
	bool found = false;
	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
	{
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			if (name != nullptr && _stricmp(name, column) == 0)
				found = true;
		}
	}
	sqlite3_finalize(stmt);
	return found;
}
long long CSpeciesDatabase::GetImageCount()
{
	if (m_db == nullptr)
		return 0;

	sqlite3_stmt* stmt = nullptr;
	long long count = 0;
	if (sqlite3_prepare_v2(m_db, "SELECT COUNT(*) FROM images;", -1, &stmt, nullptr) == SQLITE_OK)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
			count = sqlite3_column_int64(stmt, 0);
	}
	sqlite3_finalize(stmt);
	return count;
}

std::vector<ImageRecord> CSpeciesDatabase::GetImages()
{
	std::vector<ImageRecord> images;
	if (m_db == nullptr)
		return images;

	sqlite3_stmt* stmt = nullptr;
	const char* sql =
		"SELECT id, COALESCE(species_id, 1), source_file, frame_index, x, y, width, height, COALESCE(age_code, 'U')"
		" FROM images ORDER BY 2, id;";
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return images;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		ImageRecord record;
		record.id = sqlite3_column_int64(stmt, 0);
		record.speciesId = sqlite3_column_int64(stmt, 1);
		record.sourceFile = reinterpret_cast<LPCWSTR>(sqlite3_column_text16(stmt, 2));
		record.frameIndex = sqlite3_column_int64(stmt, 3);
		record.x = sqlite3_column_int(stmt, 4);
		record.y = sqlite3_column_int(stmt, 5);
		record.width = sqlite3_column_int(stmt, 6);
		record.height = sqlite3_column_int(stmt, 7);
		record.ageCode = reinterpret_cast<LPCWSTR>(sqlite3_column_text16(stmt, 8));
		images.push_back(record);
	}
	sqlite3_finalize(stmt);
	return images;
}
bool CSpeciesDatabase::GetImagePng(long long imageId, std::vector<uchar>& png)
{
	png.clear();
	if (m_db == nullptr)
		return false;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, "SELECT image_png FROM images WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK)
	{
		SetError("prepare");
		return false;
	}
	sqlite3_bind_int64(stmt, 1, imageId);
	bool ok = false;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		const uchar* data = static_cast<const uchar*>(sqlite3_column_blob(stmt, 0));
		const int size = sqlite3_column_bytes(stmt, 0);
		if (data != nullptr && size > 0)
		{
			png.assign(data, data + size);
			ok = true;
		}
	}
	sqlite3_finalize(stmt);
	return ok;
}
bool CSpeciesDatabase::Exec(const char* sql)
{
	char* message = nullptr;
	if (sqlite3_exec(m_db, sql, nullptr, nullptr, &message) != SQLITE_OK)
	{
		m_lastError = CString(message != nullptr ? message : "unknown error");
		sqlite3_free(message);
		return false;
	}
	return true;
}

void CSpeciesDatabase::SetError(const char* context)
{
	CString detail(m_db != nullptr ? sqlite3_errmsg(m_db) : "unknown error");
	m_lastError.Format(_T("%s: %s"), static_cast<LPCTSTR>(CString(context)), static_cast<LPCTSTR>(detail));
}
