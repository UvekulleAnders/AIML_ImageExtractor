// WikipediaLookup.h : looks up species names on Wikipedia (via WinHTTP)
//

#pragma once

namespace WikipediaLookup
{
	enum class NameLanguage
	{
		Swedish,
		Latin,
		English
	};

	// Uses one supplied species name to find the other names on Wikipedia/Wikidata.
	bool FindOtherNames(const CString& sourceName, NameLanguage sourceLanguage,
		CString& swedishName, CString& latinName, CString& englishName, CString& error);

	// Searches Swedish Wikipedia for the article named swedishName and returns the
	// scientific (Latin) name of the species it describes.
	bool FindLatinName(const CString& swedishName, CString& latinName, CString& error);

	// Looks the Latin name up on English Wikipedia and returns the article title,
	// which is the English common name of the species.
	bool FindEnglishName(const CString& latinName, CString& englishName, CString& error);
}
