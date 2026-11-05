#include "pch.h"
#include <gtest/gtest.h>
#include <fstream>
#include "BatchImageList.h"
#include "TestUtil.h"

namespace
{
	std::filesystem::path WriteList(const std::filesystem::path& folder, const std::string& content)
	{
		std::filesystem::create_directories(folder);
		const std::filesystem::path file = folder / L"list.txt";
		std::ofstream out(file, std::ios::binary);
		out << content;
		return file;
	}

	std::filesystem::path TempFolder(const wchar_t* name)
	{
		return std::filesystem::temp_directory_path() / name;
	}
}

TEST(BatchImageListTests, ParsesPathsSkippingBlankAndCommentLines)
{
	const auto folder = TempFolder(L"aiml_batch_parse");
	const auto file = WriteList(folder, "\xEF\xBB\xBF# header\r\nC:\\pics\\a.JPG\r\n\r\n  \"C:\\pics\\b.jpg\"  \r\nrel\\c.jpg");
	std::vector<std::wstring> images;
	ASSERT_TRUE(LoadImageList(file.wstring(), images));
	ASSERT_EQ(3u, images.size());
	EXPECT_EQ(L"C:\\pics\\a.JPG", images[0]);
	EXPECT_EQ(L"C:\\pics\\b.jpg", images[1]);
	EXPECT_EQ((folder / L"rel" / L"c.jpg").lexically_normal().wstring(), images[2]);
	std::filesystem::remove_all(folder);
}

TEST(BatchImageListTests, MissingListFileFails)
{
	std::vector<std::wstring> images;
	EXPECT_FALSE(LoadImageList(L"C:\\no\\such\\list.txt", images));
}

TEST(BatchImageListTests, ProgressRoundTripAndClear)
{
	const auto folder = TempFolder(L"aiml_batch_progress");
	const auto file = WriteList(folder, "a.jpg\n");
	EXPECT_EQ(0u, LoadBatchProgress(file.wstring()));
	ASSERT_TRUE(SaveBatchProgress(file.wstring(), 42));
	EXPECT_EQ(42u, LoadBatchProgress(file.wstring()));
	ClearBatchProgress(file.wstring());
	EXPECT_EQ(0u, LoadBatchProgress(file.wstring()));
	std::filesystem::remove_all(folder);
}
