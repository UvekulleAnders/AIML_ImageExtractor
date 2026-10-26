// TestUtil.h : helpers shared by the unit tests
#pragma once

#include "pch.h"
#include <filesystem>
#include <gtest/gtest.h>

// A unique, empty temporary folder that is removed when the object goes out of scope.
class TempDir
{
public:
	TempDir()
	{
		const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
		m_path = std::filesystem::temp_directory_path() /
			(std::string("aiml_") + info->test_suite_name() + "_" + info->name() + "_" + std::to_string(::GetCurrentProcessId()));
		std::filesystem::remove_all(m_path);
		std::filesystem::create_directories(m_path);
	}
	~TempDir()
	{
		std::error_code ec;
		std::filesystem::remove_all(m_path, ec);
	}
	const std::filesystem::path& Path() const { return m_path; }
	CString File(const wchar_t* name) const { return CString((m_path / name).c_str()); }

private:
	std::filesystem::path m_path;
};
