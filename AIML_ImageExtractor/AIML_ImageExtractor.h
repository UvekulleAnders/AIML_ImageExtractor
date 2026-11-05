
// AIML_ImageExtractor.h : main header file for the AIML_ImageExtractor application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "SpeciesDatabase.h"
#include <string>
#include <vector>

// Posted to the main frame to open the next image of a ProcessImagesFromFile run.
constexpr UINT WM_BATCH_OPEN_NEXT = WM_APP + 41;


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
	bool m_batchActive = false;
	std::wstring m_batchListFile;
	std::vector<std::wstring> m_batchImages;
	size_t m_batchNext = 0;
	size_t m_batchCurrent = 0;
	const CDocument* m_batchDoc = nullptr;
	void FinishBatch(const CString& message);


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	afx_msg void OnFileOpen();
	afx_msg void OnFileExportDataset();
	afx_msg void OnFileEditDatabase();
	afx_msg void OnFileProcessImagesFromFile();
	afx_msg void OnFileStopProcessImages();
	afx_msg void OnUpdateFileStopProcessImages(CCmdUI* pCmdUI);

	// ProcessImagesFromFile: opens the next listed image (called after the previous one was closed).
	void OpenNextBatchImage();
	// Called by a document that is being closed; advances the batch if it was the current image.
	void OnDocumentClosing(const CDocument* doc);
	bool IsBatchDocument(const CDocument* doc) const { return m_batchActive && doc == m_batchDoc; }
	size_t GetBatchPosition() const { return m_batchCurrent + 1; }
	size_t GetBatchCount() const { return m_batchImages.size(); }

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
