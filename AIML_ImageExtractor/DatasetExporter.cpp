#include "pch.h"
#include "framework.h"
#include "DatasetExporter.h"
#include <map>
#include <set>
#include <fstream>

namespace
{
	CString SafeName(CString text)
	{
		for (int i = 0; i < text.GetLength(); ++i)
		{
			const TCHAR ch = text[i];
			if (ch == _T(' '))
				text.SetAt(i, _T('_'));
			else if (wcschr(L"\\/:*?\"<>|,;", ch) != nullptr)
				text.SetAt(i, _T('-'));
		}
		return text;
	}

	CString CsvField(const CString& value)
	{
		CString escaped(value);
		escaped.Replace(_T("\""), _T("\"\""));
		return _T("\"") + escaped + _T("\"");
	}

	std::string Utf8(const CString& text)
	{
		return std::string(CW2A(text, CP_UTF8));
	}
}

int ExportDataset(CSpeciesDatabase& database, const CString& folder, const std::vector<long long>& selectedSpeciesIds, CString& error)
{
	const std::set<long long> selectedSpecies(selectedSpeciesIds.begin(), selectedSpeciesIds.end());
	std::map<long long, SpeciesRecord> species;
	for (const SpeciesRecord& s : database.GetSpecies())
		species[s.id] = s;

	if (!::CreateDirectory(folder, nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS)
	{
		error = _T("Could not create the export folder.");
		return -1;
	}

	std::ofstream manifest(std::wstring(folder + _T("\\manifest.csv")), std::ios::binary);
	if (!manifest)
	{
		error = _T("Could not write manifest.csv.");
		return -1;
	}
	manifest << "\xEF\xBB\xBF" << "image_id,file,species_id,swedish,latin,english,source_file,frame,x,y,w,h\r\n";

	int count = 0;
	for (const ImageRecord& image : database.GetImages())
	{
		if (selectedSpecies.find(image.speciesId) == selectedSpecies.end())
			continue;
		const auto speciesIt = species.find(image.speciesId);
		if (speciesIt == species.end())
			continue;
		const SpeciesRecord& sp = speciesIt->second;

		CString className = SafeName(sp.swedishName);
		if (!sp.latinName.IsEmpty())
			className += _T("__") + SafeName(sp.latinName);
		::CreateDirectory(folder + _T("\\") + className, nullptr);

		std::vector<uchar> png;
		if (!database.GetImagePng(image.id, png))
			continue;
		CString relative;
		relative.Format(_T("%s/img%lld.png"), static_cast<LPCTSTR>(className), image.id);
		CString path = folder + _T("\\") + className;
		CString fileName;
		fileName.Format(_T("\\img%lld.png"), image.id);
		std::ofstream file(std::wstring(path + fileName), std::ios::binary);
		if (!file)
		{
			error = _T("Could not write ") + path + fileName;
			return -1;
		}
		file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));

		CString line;
		line.Format(_T("%lld,%s,%lld,%s,%s,%s,%s,%lld,%d,%d,%d,%d\r\n"), image.id, static_cast<LPCTSTR>(CsvField(relative)),
			image.speciesId, static_cast<LPCTSTR>(CsvField(sp.swedishName)), static_cast<LPCTSTR>(CsvField(sp.latinName)),
			static_cast<LPCTSTR>(CsvField(sp.englishName)), static_cast<LPCTSTR>(CsvField(image.sourceFile)),
			image.frameIndex, image.x, image.y, image.width, image.height);
		manifest << Utf8(line);
		++count;
	}
	return count;
}

int ExportDataset(CSpeciesDatabase& database, const CString& folder, bool includeUnknown, CString& error)
{
	std::vector<long long> selectedSpeciesIds;
	for (const SpeciesRecord& species : database.GetSpecies())
	{
		if (includeUnknown || species.id != kUnknownSpeciesId)
			selectedSpeciesIds.push_back(species.id);
	}
	return ExportDataset(database, folder, selectedSpeciesIds, error);
}
