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
	EXPECT_FALSE(db.AddSpecies(Species(L"Blames", L"x", L"y")));
	EXPECT_FALSE(db.GetLastError().IsEmpty());
}

TEST(SpeciesDatabase, SpeciesAreOrderedWithUnknownFirst)
{
	TempDir dir;
	CSpeciesDatabase db;
	ASSERT_TRUE(db.Open(dir.File(L"test.db")));
	ASSERT_TRUE(db.AddSpecies(Species(L"Talgoxe", L"", L"")));
	ASSERT_TRUE(db.AddSpecies(Species(L"Blames", L"", L"")));
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
