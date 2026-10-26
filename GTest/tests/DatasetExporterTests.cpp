#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include "DatasetExporter.h"
#include "TestUtil.h"

namespace
{
	std::string ReadAll(const std::filesystem::path& path)
	{
		std::ifstream in(path, std::ios::binary);
		std::stringstream ss;
		ss << in.rdbuf();
		return ss.str();
	}

	long long AddBird(CSpeciesDatabase& db, const wchar_t* swedish, const wchar_t* latin)
	{
		SpeciesRecord s;
		s.swedishName = swedish;
		s.latinName = latin;
		s.englishName = L"english";
		long long id = 0;
		EXPECT_TRUE(db.AddSpecies(s, &id));
		return id;
	}
}

TEST(DatasetExporter, WritesClassFoldersPngsAndManifest)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	const long long tit = AddBird(db, L"Bl\u00e5mes", L"Cyanistes caeruleus");
	const std::vector<uchar> png = { 1, 2, 3, 4 };
	long long imageId = 0;
	ASSERT_TRUE(db.AddImage(L"clip.h264", 17, cv::Rect(5, 6, 7, 8), png, tit, &imageId));

	CString error;
	const CString out = dir.File(L"export");
	ASSERT_EQ(1, ExportDataset(db, out, true, error)) << CStringA(error).GetString();

	const auto folder = dir.Path() / "export" / L"Bl\u00e5mes__Cyanistes_caeruleus";
	const auto file = folder / ("img" + std::to_string(imageId) + ".png");
	ASSERT_TRUE(std::filesystem::exists(file));
	EXPECT_EQ(std::string("\x01\x02\x03\x04", 4), ReadAll(file));

	const std::string manifest = ReadAll(dir.Path() / "export" / "manifest.csv");
	EXPECT_EQ(0, manifest.compare(0, 3, "\xEF\xBB\xBF")); // UTF-8 BOM
	EXPECT_NE(std::string::npos, manifest.find("image_id,file,species_id,swedish,latin,english,source_file,frame,x,y,w,h"));
	EXPECT_NE(std::string::npos, manifest.find("clip.h264"));
	EXPECT_NE(std::string::npos, manifest.find(",17,5,6,7,8"));
}

TEST(DatasetExporter, UnknownIsOptional)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	const long long tit = AddBird(db, L"Talgoxe", L"Parus major");
	ASSERT_TRUE(db.AddImage(L"v", 1, cv::Rect(0, 0, 8, 8), { 1 }));
	ASSERT_TRUE(db.AddImage(L"v", 2, cv::Rect(0, 0, 8, 8), { 2 }, tit));

	CString error;
	EXPECT_EQ(1, ExportDataset(db, dir.File(L"without"), false, error));
	EXPECT_EQ(2, ExportDataset(db, dir.File(L"with"), true, error));
}

TEST(DatasetExporter, EmptyDatabaseWritesHeaderOnly)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	CString error;
	EXPECT_EQ(0, ExportDataset(db, dir.File(L"export"), true, error));
	EXPECT_TRUE(std::filesystem::exists(dir.Path() / "export" / "manifest.csv"));
}

TEST(DatasetExporter, InvalidFolderFails)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	CString error;
	EXPECT_EQ(-1, ExportDataset(db, dir.File(L"missing\\deeper\\export"), true, error));
	EXPECT_FALSE(error.IsEmpty());
}

TEST(DatasetExporter, UnsafeCharactersInNamesAreReplaced)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	const long long id = AddBird(db, L"A/B:C", L"x y");
	ASSERT_TRUE(db.AddImage(L"v", 1, cv::Rect(0, 0, 8, 8), { 1 }, id));
	CString error;
	ASSERT_EQ(1, ExportDataset(db, dir.File(L"export"), true, error));
	EXPECT_TRUE(std::filesystem::is_directory(dir.Path() / "export" / "A-B-C__x_y"));
}
