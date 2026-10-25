
// AIML_ImageExtractorDoc.h : interface of the CAIMLImageExtractorDoc class
//


#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include "ObjectDetector.h"


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
	// Cuts the region out of the current frame and stores it in the species database.
	bool ExtractRegion(const cv::Rect& region);
	// Cuts out several regions (all assigned to one species chosen once) and stores them.
	bool ExtractRegions(const std::vector<cv::Rect>& regions);
	// Finds birds in the current frame; the boxes are kept until the frame changes.
	bool DetectObjects();
	const std::vector<cv::Rect>& GetDetections() const { return m_detections; }
	// Automatic generation: stores one image per detected bird in the current frame.
	bool AutoExtract();
	bool GetAutoDetect() const { return m_autoDetect; }
	void SetAutoDetect(bool on);
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
	CObjectDetector m_detector;
	std::vector<cv::Rect> m_detections;
	bool m_autoDetect = false;
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
