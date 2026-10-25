
// AIML_ImageExtractorDoc.cpp : implementation of the CAIMLImageExtractorDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AIML_ImageExtractor.h"
#endif

#include "AIML_ImageExtractorDoc.h"
#include "MainFrm.h"

#include <propkey.h>
#include <atlconv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAIMLImageExtractorDoc

IMPLEMENT_DYNCREATE(CAIMLImageExtractorDoc, CDocument)

BEGIN_MESSAGE_MAP(CAIMLImageExtractorDoc, CDocument)
END_MESSAGE_MAP()


// CAIMLImageExtractorDoc construction/destruction

CAIMLImageExtractorDoc::CAIMLImageExtractorDoc() noexcept
{
	// TODO: add one-time construction code here
}

CAIMLImageExtractorDoc::~CAIMLImageExtractorDoc()
{
	m_video.release();
}

BOOL CAIMLImageExtractorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	m_video.release();
	m_currentFrame.release();
	m_frameCount = 0.0;
	m_currentFrameIndex = 0.0;

	CW2A filePath(lpszPathName);
	m_filePath = static_cast<LPCSTR>(filePath);
	m_seekable = true;
	if (!ReopenVideo())
	{
		AfxMessageBox(_T("OpenCV could not open this video file."), MB_ICONERROR);
		return FALSE;
	}

	m_frameCount = m_video.get(cv::CAP_PROP_FRAME_COUNT);
	if (m_frameCount <= 0.0)
	{
		// Raw streams (e.g. .h264) carry no frame count or seek index, so count by decoding.
		CWaitCursor waitCursor;
		m_seekable = false;
		double frames = 0.0;
		while (m_video.grab())
			frames += 1.0;
		m_frameCount = frames;
		if (!ReopenVideo())
		{
			AfxMessageBox(_T("OpenCV could not reopen this video file."), MB_ICONERROR);
			return FALSE;
		}
	}
	if (!MoveToFrame(0.0))
	{
		m_video.release();
		AfxMessageBox(_T("The video opened, but no frame could be read."), MB_ICONERROR);
		return FALSE;
	}

	CFileStatus fileStatus;
	ULONGLONG fileSize = CFile::GetStatus(lpszPathName, fileStatus) ? fileStatus.m_size : 0;
	CMainFrame* mainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	if (mainFrame != nullptr)
		mainFrame->SetVideoProperties(lpszPathName, GetVideoCodec(), fileSize, m_frameCount);

	UpdateAllViews(nullptr);
	return TRUE;
}

bool CAIMLImageExtractorDoc::MoveToFrame(double frameIndex)
{
	if (!m_video.isOpened() || m_frameCount <= 0.0)
		return false;
	if (frameIndex < 0)
		frameIndex = 0;
	if (frameIndex >= m_frameCount)
		frameIndex = m_frameCount - 1;

	cv::Mat frame;
	if (m_seekable)
	{
		if (!m_video.set(cv::CAP_PROP_POS_FRAMES, frameIndex))
			return false;
	}
	else
	{
		// Sequential-only stream: rewind by reopening, then decode forward to the target.
		if (frameIndex < m_nextFrameIndex && !ReopenVideo())
			return false;
		while (m_nextFrameIndex < frameIndex)
		{
			if (!m_video.grab())
				return false;
			m_nextFrameIndex += 1.0;
		}
	}
	if (!m_video.read(frame) || frame.empty())
		return false;

	m_nextFrameIndex = frameIndex + 1.0;
	m_currentFrame = frame;
	m_currentFrameIndex = frameIndex;
	UpdateAllViews(nullptr);
	return true;
}

bool CAIMLImageExtractorDoc::ReopenVideo()
{
	m_video.release();
	m_nextFrameIndex = 0.0;
	return m_video.open(std::string(m_filePath.GetString()), cv::CAP_FFMPEG);
}

bool CAIMLImageExtractorDoc::ExtractRegion(const cv::Rect& region)
{
	CFrameWnd* mainFrame = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
	const cv::Rect bounds(0, 0, m_currentFrame.cols, m_currentFrame.rows);
	const cv::Rect clipped = region & bounds;
	if (m_currentFrame.empty() || clipped.empty())
		return false;

	CSpeciesDatabase& database = theApp.GetSpeciesDatabase();
	if (!database.IsOpen())
	{
		if (mainFrame != nullptr)
			mainFrame->SetMessageText(_T("The species database is not open; the image was not stored."));
		return false;
	}

	std::vector<uchar> png;
	if (!cv::imencode(".png", m_currentFrame(clipped), png))
		return false;

	long long imageId = 0;
	const bool stored = database.AddImage(GetPathName(), static_cast<long long>(m_currentFrameIndex), clipped, png, &imageId);
	CString message;
	if (stored)
	{
		message.Format(_T("Stored image %lld (%dx%d) from frame %.0f in speciesImages.db"),
			imageId, clipped.width, clipped.height, m_currentFrameIndex);
	}
	else
	{
		message.Format(_T("Could not store the image: %s"), static_cast<LPCTSTR>(database.GetLastError()));
	}
	if (mainFrame != nullptr)
		mainFrame->SetMessageText(message);
	return stored;
}

CString CAIMLImageExtractorDoc::GetVideoCodec() const
{
	const int fourcc = static_cast<int>(m_video.get(cv::CAP_PROP_FOURCC));
	if (fourcc == 0)
		return _T("Unknown");

	CString codec;
	codec.Format(_T("%c%c%c%c"), fourcc & 0xff, (fourcc >> 8) & 0xff,
		(fourcc >> 16) & 0xff, (fourcc >> 24) & 0xff);
	return codec;
}

BOOL CAIMLImageExtractorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CAIMLImageExtractorDoc serialization

void CAIMLImageExtractorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CAIMLImageExtractorDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CAIMLImageExtractorDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CAIMLImageExtractorDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CAIMLImageExtractorDoc diagnostics

#ifdef _DEBUG
void CAIMLImageExtractorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAIMLImageExtractorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CAIMLImageExtractorDoc commands

namespace
{
	long ClampFrameIndex(long frameIndex, long frameCount)
	{
		if (frameIndex < 0)
			frameIndex = 0;
		if (frameCount <= 0)
			return 0;
		if (frameIndex >= frameCount)
			frameIndex = frameCount - 1;
		return frameIndex;
	}
}
