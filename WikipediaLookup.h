// WikipediaLookup.h : looks up species names on Wikipedia (via WinHTTP)
//

#pragma once

namespace WikipediaLookup
{
	// Searches Swedish Wikipedia for the article named swedishName and returns the
	// scientific (Latin) name of the species it describes.
	bool FindLatinName(const CString& swedishName, CString& latinName, CString& error);

	// Looks the Latin name up on English Wikipedia and returns the article title,
	// which is the English common name of the species.
	bool FindEnglishName(const CString& latinName, CString& englishName, CString& error);
}
