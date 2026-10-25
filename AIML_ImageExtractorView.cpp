
// AIML_ImageExtractorView.cpp : implementation of the CAIMLImageExtractorView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AIML_ImageExtractor.h"
#endif

#include "AIML_ImageExtractorDoc.h"
#include "AIML_ImageExtractorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAIMLImageExtractorView

IMPLEMENT_DYNCREATE(CAIMLImageExtractorView, CView)

BEGIN_MESSAGE_MAP(CAIMLImageExtractorView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CAIMLImageExtractorView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CAIMLImageExtractorView construction/destruction

CAIMLImageExtractorView::CAIMLImageExtractorView() noexcept
{
	// TODO: add construction code here

}

CAIMLImageExtractorView::~CAIMLImageExtractorView()
{
}

BOOL CAIMLImageExtractorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style |= WS_TABSTOP;
	return CView::PreCreateWindow(cs);
}

void CAIMLImageExtractorView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	SetFocus();
}

// CAIMLImageExtractorView drawing

void CAIMLImageExtractorView::OnDraw(CDC* pDC)
{
	CAIMLImageExtractorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	const cv::Mat& frame = pDoc->GetCurrentFrame();
	CRect clientRect;
	GetClientRect(&clientRect);
	pDC->FillSolidRect(clientRect, RGB(0, 0, 0));
	if (frame.empty() || clientRect.IsRectEmpty())
		return;

	cv::Mat bgraFrame;
	if (frame.channels() == 4)
		bgraFrame = frame;
	else if (frame.channels() == 3)
		cv::cvtColor(frame, bgraFrame, cv::COLOR_BGR2BGRA);
	else if (frame.channels() == 1)
		cv::cvtColor(frame, bgraFrame, cv::COLOR_GRAY2BGRA);
	else
		return;

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = bgraFrame.cols;
	bitmapInfo.bmiHeader.biHeight = -bgraFrame.rows;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	double scaleX = static_cast<double>(clientRect.Width()) / bgraFrame.cols;
	double scaleY = static_cast<double>(clientRect.Height()) / bgraFrame.rows;
	double scale = scaleX < scaleY ? scaleX : scaleY;
	int drawWidth = static_cast<int>(bgraFrame.cols * scale);
	int drawHeight = static_cast<int>(bgraFrame.rows * scale);
	int left = clientRect.left + (clientRect.Width() - drawWidth) / 2;
	int top = clientRect.top + (clientRect.Height() - drawHeight) / 2;

	::SetStretchBltMode(pDC->GetSafeHdc(), HALFTONE);
	::StretchDIBits(pDC->GetSafeHdc(), left, top, drawWidth, drawHeight,
		0, 0, bgraFrame.cols, bgraFrame.rows, bgraFrame.data,
		&bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

void CAIMLImageExtractorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CAIMLImageExtractorDoc* pDoc = GetDocument();
	if (pDoc != nullptr)
	{
		double currentFrame = pDoc->GetCurrentFrameIndex();
		double lastFrame = pDoc->GetFrameCount() - 1.0;
		switch (nChar)
		{
		case VK_LEFT:
			pDoc->MoveToFrame(currentFrame - 1.0);
			return;
		case VK_RIGHT:
			pDoc->MoveToFrame(currentFrame + 1.0);
			return;
		case VK_HOME:
			pDoc->MoveToFrame(0.0);
			return;
		case VK_END:
			pDoc->MoveToFrame(lastFrame);
			return;
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


// CAIMLImageExtractorView printing


void CAIMLImageExtractorView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CAIMLImageExtractorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CAIMLImageExtractorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CAIMLImageExtractorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CAIMLImageExtractorView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CAIMLImageExtractorView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CAIMLImageExtractorView diagnostics

#ifdef _DEBUG
void CAIMLImageExtractorView::AssertValid() const
{
	CView::AssertValid();
}

void CAIMLImageExtractorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CAIMLImageExtractorDoc* CAIMLImageExtractorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAIMLImageExtractorDoc)));
	return (CAIMLImageExtractorDoc*)m_pDocument;
}
#endif //_DEBUG


// CAIMLImageExtractorView message handlers
