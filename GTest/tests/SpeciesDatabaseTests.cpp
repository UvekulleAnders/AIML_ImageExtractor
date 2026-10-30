#include "pch.h"
#include <gtest/gtest.h>
#include "SpeciesDatabase.h"
#include "TestUtil.h"

namespace
{
	SpeciesRecord Species(const wchar_t* swedish, const wchar_t* latin, const wchar_t* english)
	{
		SpeciesRecord s;
		s.swedishName = swedish;
		s.latinName = latin;
		s.englishName = english;
		return s;
	}
}

TEST(SpeciesDatabase, OpenCreatesFileAndUnknownSpecies)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	EXPECT_TRUE(db.IsOpen());
	EXPECT_TRUE(std::filesystem::exists(dir.Path() / "test.db"));

	const auto species = db.GetSpecies();
	ASSERT_EQ(1u, species.size());
	EXPECT_EQ(kUnknownSpeciesId, species[0].id);
	EXPECT_EQ(CString(L"Unknown"), species[0].swedishName);
	EXPECT_EQ(0, db.GetImageCount());
}

TEST(SpeciesDatabase, ReopenKeepsData)
{
	TempDir dir;
	{
		CSpeciesDatabase db;
		ASSERT_TRUE(db.Open(dir.File(L"test.db")));
		ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"Parus major", L"Great tit")));
	}
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	EXPECT_EQ(2u, db.GetSpecies().size());
}

TEST(SpeciesDatabase, AddSpeciesReturnsIdAndRejectsDuplicates)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long id = 0;
	ASSERT_TRUE(db.AddSpecies(Species(L"Blames", L"Cyanistes caeruleus", L"Eurasian blue tit"), &id));
	EXPECT_GT(id, kUnknownSpeciesId);
	EXPECT_FALSE(db.AddSpecies(Species(L"Other", L"cyanistes CAERULEUS", L"y")));
	EXPECT_FALSE(db.AddSpecies(Species(L"Blames", L"", L"y")));
	EXPECT_FALSE(db.GetLastError().IsEmpty());
}

TEST(SpeciesDatabase, SpeciesAreOrderedWithUnknownFirst)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"Parus major", L"")));
	ASSERT_TRUE(db.AddSpecies(Species(L"Blames", L"Cyanistes caeruleus", L"")));
	const auto species = db.GetSpecies();
	ASSERT_EQ(3u, species.size());
	EXPECT_EQ(kUnknownSpeciesId, species[0].id);
	EXPECT_EQ(CString(L"Blames"), species[1].swedishName);
	EXPECT_EQ(CString(L"Talgoxe"), species[2].swedishName);
}

TEST(SpeciesDatabase, ImageRoundTrip)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long speciesId = 0;
	ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"Parus major", L"Great tit"), &speciesId));

	const std::vector<uchar> png = { 0x89, 'P', 'N', 'G', 1, 2, 3, 0, 255 };
	long long imageId = 0;
	ASSERT_TRUE(db.AddImage(L"video.h264", 480, cv::Rect(10, 20, 30, 40), png, speciesId, &imageId));
	EXPECT_EQ(1, db.GetImageCount());

	const auto images = db.GetImages();
	ASSERT_EQ(1u, images.size());
	EXPECT_EQ(imageId, images[0].id);
	EXPECT_EQ(speciesId, images[0].speciesId);
	EXPECT_EQ(CString(L"video.h264"), images[0].sourceFile);
	EXPECT_EQ(480, images[0].frameIndex);
	EXPECT_EQ(10, images[0].x);
	EXPECT_EQ(20, images[0].y);
	EXPECT_EQ(30, images[0].width);
	EXPECT_EQ(40, images[0].height);

	std::vector<uchar> stored;
	ASSERT_TRUE(db.GetImagePng(imageId, stored));
	EXPECT_EQ(png, stored);
}

TEST(SpeciesDatabase, ImageDefaultsToUnknownSpecies)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	ASSERT_TRUE(db.AddImage(L"v.h264", 1, cv::Rect(0, 0, 8, 8), { 1, 2, 3 }));
	EXPECT_EQ(kUnknownSpeciesId, db.GetImages().at(0).speciesId);
}

TEST(SpeciesDatabase, MissingImageIsNotFound)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	std::vector<uchar> png;
	EXPECT_FALSE(db.GetImagePng(12345, png));
}

TEST(SpeciesDatabase, ClosedDatabaseFailsGracefully)
{
	CSpeciesDatabase db;
	EXPECT_FALSE(db.IsOpen());
	EXPECT_FALSE(db.AddImage(L"v", 0, cv::Rect(0, 0, 1, 1), { 1 }));
	EXPECT_TRUE(db.GetSpecies().empty());
}

TEST(SpeciesDatabase, OpenFailsForUnreachablePath)
{
	TempDir dir;
	CSpeciesDatabase db;
	EXPECT_FALSE(db.Open(dir.File(L"no_such_folder\\test.db")));
	EXPECT_FALSE(db.IsOpen());
}

TEST(SpeciesDatabase, UpdateSpeciesChangesNamesAndRejectsDuplicatesAndUnknown)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long a = 0, b = 0;
	ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"Parus major", L""), &a));
	ASSERT_TRUE(db.AddSpecies(Species(L"Blames", L"Cyanistes caeruleus", L""), &b));

	SpeciesRecord edited = Species(L"Talgoxe", L"Parus major", L"Great tit");
	edited.id = a;
	ASSERT_TRUE(db.UpdateSpecies(edited));
	bool found = false;
	for (const SpeciesRecord& s : db.GetSpecies())
	{
		if (s.id == a)
		{
			found = true;
			EXPECT_EQ(CString(L"Parus major"), s.latinName);
			EXPECT_EQ(CString(L"Great tit"), s.englishName);
		}
	}
	EXPECT_TRUE(found);

	edited.latinName = L"cyanistes caeruleus";
	EXPECT_FALSE(db.UpdateSpecies(edited));
	edited.latinName = L"";
	EXPECT_FALSE(db.UpdateSpecies(edited));
	edited.latinName = L"Parus major";
	edited.id = kUnknownSpeciesId;
	edited.swedishName = L"Other";
	EXPECT_FALSE(db.UpdateSpecies(edited));
	edited.id = 9999;
	EXPECT_FALSE(db.UpdateSpecies(edited));
}

TEST(SpeciesDatabase, LatinNameIsCompulsoryButSwedishAndEnglishAreOptional)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long a = 0, b = 0;
	EXPECT_FALSE(db.AddSpecies(Species(L"Talgoxe", L"", L"Great tit")));
	ASSERT_TRUE(db.AddSpecies(Species(L"", L"Parus major", L""), &a));
	ASSERT_TRUE(db.AddSpecies(Species(L"", L"Cyanistes caeruleus", L""), &b));

	SpeciesRecord edit = Species(L"Talgoxe", L"Parus major", L"");
	edit.id = a;
	EXPECT_TRUE(db.UpdateSpecies(edit));
	edit.swedishName = L"";
	edit.englishName = L"Great tit";
	EXPECT_TRUE(db.UpdateSpecies(edit));
	edit.latinName = L"";
	EXPECT_FALSE(db.UpdateSpecies(edit));
}

TEST(SpeciesDatabase, OldDatabaseWithUniqueSwedishNameIsMigratedKeepingImages)
{
	TempDir dir;
	const CString path = dir.File(L"old.db");
	sqlite3* raw = nullptr;
	ASSERT_EQ(SQLITE_OK, sqlite3_open16(path.GetString(), &raw));
	ASSERT_EQ(SQLITE_OK, sqlite3_exec(raw,
		"CREATE TABLE species (id INTEGER PRIMARY KEY AUTOINCREMENT, swedish_name TEXT NOT NULL UNIQUE, latin_name TEXT, english_name TEXT, created_at TEXT NOT NULL DEFAULT (datetime('now')));"
		"CREATE TABLE images (id INTEGER PRIMARY KEY AUTOINCREMENT, species_id INTEGER REFERENCES species(id) ON DELETE SET NULL, source_file TEXT NOT NULL, frame_index INTEGER NOT NULL, x INTEGER NOT NULL, y INTEGER NOT NULL, width INTEGER NOT NULL, height INTEGER NOT NULL, image_png BLOB NOT NULL, created_at TEXT NOT NULL DEFAULT (datetime('now')));"
		"INSERT INTO species (id, swedish_name, latin_name, english_name) VALUES (1, 'Unknown', ', 'Unknown'), (2, 'Talgoxe', 'Parus major', 'Great tit');"
		"INSERT INTO images (species_id, source_file, frame_index, x, y, width, height, image_png) VALUES (2, 'v', 1, 0, 0, 8, 8, x'01');",
		nullptr, nullptr, nullptr));
	sqlite3_close(raw);

	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(path));
	ASSERT_EQ(1u, db.GetImages().size());
	EXPECT_EQ(2, db.GetImages()[0].speciesId);
	EXPECT_TRUE(db.AddSpecies(Species(L"", L"Cyanistes caeruleus", L"")));
	EXPECT_TRUE(db.AddSpecies(Species(L"", L"Corvus corax", L"")));
}

TEST(SpeciesDatabase, ImageAgeDefaultsToUnknownAndCanBeSetToValidCodes)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long image = 0;
	ASSERT_TRUE(db.AddImage(L"v", 1, cv::Rect(0, 0, 8, 8), { 1 }, kUnknownSpeciesId, &image));
	EXPECT_EQ(CString(L"U"), db.GetImages()[0].ageCode);

	EXPECT_TRUE(db.SetImageAge(image, L"HY"));
	EXPECT_EQ(CString(L"HY"), db.GetImages()[0].ageCode);
	EXPECT_FALSE(db.SetImageAge(image, L"XYZ"));
	EXPECT_FALSE(db.SetImageAge(9999, L"SY"));
}

TEST(SpeciesDatabase, SetImageSpeciesRelinksImage)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	long long species = 0, image = 0;
	ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"Parus major", L""), &species));
	ASSERT_TRUE(db.AddImage(L"v", 1, cv::Rect(0, 0, 8, 8), { 1 }, kUnknownSpeciesId, &image));

	ASSERT_TRUE(db.SetImageSpecies(image, species));
	ASSERT_EQ(1u, db.GetImages().size());
	EXPECT_EQ(species, db.GetImages()[0].speciesId);
	EXPECT_FALSE(db.SetImageSpecies(image, 9999));
	EXPECT_FALSE(db.SetImageSpecies(9999, species));
}