// AgeCodes.h : USGS Bird Banding Laboratory age (year class) codes
// https://www.pwrc.usgs.gov/BBL/Bander_Portal/login/age.php
//

#pragma once

#include <afxstr.h>

struct AgeCode
{
	const wchar_t* code;
	const wchar_t* name;
	const wchar_t* description;
};

// Default is U (Unknown). J is obsolete in the BBL scheme but remains valid for old records.
inline const AgeCode* GetAgeCodes(int& count)
{
	static const AgeCode codes[] = {
		{ L"U", L"Unknown", L"Cannot be placed in any other class." },
		{ L"AHY", L"After Hatching Year", L"Hatched before the calendar year of banding; year of hatch otherwise unknown." },
		{ L"HY", L"Hatching Year", L"Capable of sustained flight and hatched during the calendar year of banding." },
		{ L"L", L"Local", L"Nestling or young bird incapable of sustained flight." },
		{ L"SY", L"Second Year", L"Hatched in the calendar year preceding the year of banding." },
		{ L"ASY", L"After Second Year", L"Hatched earlier than the calendar year preceding the year of banding." },
		{ L"TY", L"Third Year", L"Hatched two calendar years before the year of banding." },
		{ L"ATY", L"After Third Year", L"Hatched more than two calendar years before the year of banding." },
		{ L"J", L"Juvenile (obsolete)", L"Old code for a nestling or recent fledgling; probably mostly translatable to L." },
	};
	count = static_cast<int>(sizeof(codes) / sizeof(codes[0]));
	return codes;
}

inline bool IsValidAgeCode(const CString& code)
{
	int count = 0;
	const AgeCode* codes = GetAgeCodes(count);
	for (int i = 0; i < count; ++i)
	{
		if (code == codes[i].code)
			return true;
	}
	return false;
}
