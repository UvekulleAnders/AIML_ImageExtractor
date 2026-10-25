
// AIML_ImageExtractor.h : main header file for the AIML_ImageExtractor application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "SpeciesDatabase.h"


// CAIMLImageExtractorApp:
// See AIML_ImageExtractor.cpp for the implementation of this class
//

class CAIMLImageExtractorApp : public CWinAppEx
{
public:
	CAIMLImageExtractorApp() noexcept;

	CSpeciesDatabase& GetSpeciesDatabase() { return m_speciesDb; }

private:
	CSpeciesDatabase m_speciesDb;


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	afx_msg void OnFileOpen();
	afx_msg void OnFileExportDataset();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CAIMLImageExtractorApp theApp;
