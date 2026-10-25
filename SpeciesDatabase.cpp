// SpeciesDatabase.cpp : implementation of the CSpeciesDatabase class
//

#include "pch.h"
#include "SpeciesDatabase.h"

namespace
{
	const char* const kSchema =
		"PRAGMA foreign_keys = ON;"
		"CREATE TABLE IF NOT EXISTS species ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  name TEXT NOT NULL UNIQUE,"
		"  latin_name TEXT,"
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
	if (!Exec(kSchema))
	{
		Close();
		return false;
	}
	return true;
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
	const std::vector<uchar>& pngData, long long* newId)
{
	if (m_db == nullptr || pngData.empty())
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql =
		"INSERT INTO images (source_file, frame_index, x, y, width, height, image_png)"
		" VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);";
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

	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	if (!ok)
		SetError("insert");
	else if (newId != nullptr)
		*newId = sqlite3_last_insert_rowid(m_db);
	sqlite3_finalize(stmt);
	return ok;
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
