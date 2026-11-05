// BatchImageList.cpp : image list files for ProcessImagesFromFile and their progress files
//

#include "pch.h"
#include "BatchImageList.h"
#include <windows.h>
#include <fstream>
#include <iterator>

namespace
{
	std::wstring Decode(const std::string& bytes)
	{
		size_t start = 0;
		if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF
			&& static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
			start = 3;
		if (start >= bytes.size())
			return std::wstring();
		const char* data = bytes.data() + start;
		const int length = static_cast<int>(bytes.size() - start);
		UINT codePage = CP_UTF8;
		int wide = ::MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, data, length, nullptr, 0);
		if (wide <= 0)
		{
			codePage = CP_ACP;
			wide = ::MultiByteToWideChar(codePage, 0, data, length, nullptr, 0);
		}
		if (wide <= 0)
			return std::wstring();
		std::wstring text(wide, L'\0');
		::MultiByteToWideChar(codePage, 0, data, length, &text[0], wide);
		return text;
	}

	std::wstring Trim(const std::wstring& value)
	{
		const wchar_t* whitespace = L" \t\r\n\"";
		const size_t first = value.find_first_not_of(whitespace);
		if (first == std::wstring::npos)
			return std::wstring();
		const size_t last = value.find_last_not_of(whitespace);
		return value.substr(first, last - first + 1);
	}
}

bool LoadImageList(const std::wstring& listFile, std::vector<std::wstring>& images)
{
	images.clear();
	std::ifstream in(listFile.c_str(), std::ios::binary);
	if (!in)
		return false;
	const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	const std::wstring text = Decode(bytes);
	const size_t slash = listFile.find_last_of(L"\\/");
	const std::wstring baseFolder = slash == std::wstring::npos ? std::wstring() : listFile.substr(0, slash + 1);

	size_t pos = 0;
	while (pos <= text.size())
	{
		size_t end = text.find(L'\n', pos);
		if (end == std::wstring::npos)
			end = text.size();
		const std::wstring line = Trim(text.substr(pos, end - pos));
		pos = end + 1;
		if (line.empty() || line[0] == L'#')
			continue;
		const bool absolute = line[0] == L'\\' || line[0] == L'/' || (line.size() > 1 && line[1] == L':');
		const std::wstring full = absolute ? line : baseFolder + line;
		wchar_t buffer[MAX_PATH * 4];
		const DWORD length = ::GetFullPathNameW(full.c_str(), static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), buffer, nullptr);
		images.push_back(length > 0 && length < sizeof(buffer) / sizeof(buffer[0]) ? std::wstring(buffer, length) : full);
	}
	return true;
}

std::wstring GetProgressFilePath(const std::wstring& listFile)
{
	return listFile + L".progress";
}

size_t LoadBatchProgress(const std::wstring& listFile)
{
	std::ifstream in(GetProgressFilePath(listFile).c_str());
	size_t count = 0;
	if (!(in >> count))
		return 0;
	return count;
}

bool SaveBatchProgress(const std::wstring& listFile, size_t processedCount)
{
	std::ofstream out(GetProgressFilePath(listFile).c_str(), std::ios::trunc);
	if (!out)
		return false;
	out << processedCount << "\n";
	return static_cast<bool>(out);
}

void ClearBatchProgress(const std::wstring& listFile)
{
	::DeleteFileW(GetProgressFilePath(listFile).c_str());
}
