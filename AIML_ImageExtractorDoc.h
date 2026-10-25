
// AIML_ImageExtractorDoc.h : interface of the CAIMLImageExtractorDoc class
//


#pragma once

#include <opencv2/opencv.hpp>


class CAIMLImageExtractorDoc : public CDocument
{
protected: // create from serialization only
	CAIMLImageExtractorDoc() noexcept;
	DECLARE_DYNCREATE(CAIMLImageExtractorDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	bool MoveToFrame(double frameIndex);
	const cv::Mat& GetCurrentFrame() const { return m_currentFrame; }
	double GetCurrentFrameIndex() const { return m_currentFrameIndex; }
	double GetFrameCount() const { return m_frameCount; }
	CString GetVideoCodec() const;
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CAIMLImageExtractorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	cv::VideoCapture m_video;
	cv::Mat m_currentFrame;
	double m_currentFrameIndex = 0;
	double m_frameCount = 0;
	double m_nextFrameIndex = 0;
	bool m_seekable = true;
	CStringA m_filePath;
	bool ReopenVideo();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
