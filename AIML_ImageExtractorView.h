
// AIML_ImageExtractorView.h : interface of the CAIMLImageExtractorView class
//

#pragma once

#include <opencv2/core.hpp>


class CAIMLImageExtractorView : public CView
{
protected: // create from serialization only
	CAIMLImageExtractorView() noexcept;
	DECLARE_DYNCREATE(CAIMLImageExtractorView)

// Attributes
public:
	CAIMLImageExtractorDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CAIMLImageExtractorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void DrawContent(CDC* pDC, const CRect& clientRect);
	bool GetFrameLayout(const CRect& clientRect, const cv::Mat& frame, CRect& drawRect, double& scale) const;
	bool ClientToFramePoint(CPoint point, CPoint& framePoint) const;

	static const int kMinSelectionSize = 4;
	bool m_selecting = false;
	CPoint m_dragStart;
	CPoint m_dragCurrent;
	bool m_hasSelection = false;
	cv::Rect m_selectionRect;
	double m_selectionFrame = -1.0;

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in AIML_ImageExtractorView.cpp
inline CAIMLImageExtractorDoc* CAIMLImageExtractorView::GetDocument() const
   { return reinterpret_cast<CAIMLImageExtractorDoc*>(m_pDocument); }
#endif

