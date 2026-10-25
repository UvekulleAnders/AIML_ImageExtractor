
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
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
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
	CRect clientRect;
	GetClientRect(&clientRect);
	if (pDC->IsPrinting() || clientRect.IsRectEmpty())
	{
		DrawContent(pDC, clientRect);
		return;
	}

	// Draw off-screen so the selection rectangle does not flicker while dragging.
	CDC memDC;
	CBitmap bitmap;
	if (!memDC.CreateCompatibleDC(pDC) || !bitmap.CreateCompatibleBitmap(pDC, clientRect.Width(), clientRect.Height()))
	{
		DrawContent(pDC, clientRect);
		return;
	}
	CBitmap* oldBitmap = memDC.SelectObject(&bitmap);
	DrawContent(&memDC, clientRect);
	pDC->BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBitmap);
}

bool CAIMLImageExtractorView::GetFrameLayout(const CRect& clientRect, const cv::Mat& frame, CRect& drawRect, double& scale) const
{
	if (frame.empty() || clientRect.IsRectEmpty())
		return false;

	const double scaleX = static_cast<double>(clientRect.Width()) / frame.cols;
	const double scaleY = static_cast<double>(clientRect.Height()) / frame.rows;
	scale = scaleX < scaleY ? scaleX : scaleY;
	const int drawWidth = static_cast<int>(frame.cols * scale);
	const int drawHeight = static_cast<int>(frame.rows * scale);
	const int left = clientRect.left + (clientRect.Width() - drawWidth) / 2;
	const int top = clientRect.top + (clientRect.Height() - drawHeight) / 2;
	drawRect.SetRect(left, top, left + drawWidth, top + drawHeight);
	return drawWidth > 0 && drawHeight > 0;
}

void CAIMLImageExtractorView::DrawContent(CDC* pDC, const CRect& clientRect)
{
	CAIMLImageExtractorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	const cv::Mat& frame = pDoc->GetCurrentFrame();
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

	CRect drawRect;
	double scale = 1.0;
	if (!GetFrameLayout(clientRect, bgraFrame, drawRect, scale))
		return;

	::SetStretchBltMode(pDC->GetSafeHdc(), HALFTONE);
	::StretchDIBits(pDC->GetSafeHdc(), drawRect.left, drawRect.top, drawRect.Width(), drawRect.Height(),
		0, 0, bgraFrame.cols, bgraFrame.rows, bgraFrame.data,
		&bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

	// Selection rectangle: the one being dragged, or the last one made on this frame.
	CRect selection;
	if (m_selecting)
	{
		selection.SetRect(m_dragStart, m_dragCurrent);
		selection.NormalizeRect();
	}
	else if (m_hasSelection && m_selectionFrame == pDoc->GetCurrentFrameIndex())
	{
		selection.SetRect(
			drawRect.left + static_cast<int>(m_selectionRect.x * scale),
			drawRect.top + static_cast<int>(m_selectionRect.y * scale),
			drawRect.left + static_cast<int>((m_selectionRect.x + m_selectionRect.width) * scale),
			drawRect.top + static_cast<int>((m_selectionRect.y + m_selectionRect.height) * scale));
	}
	if (!selection.IsRectEmpty())
	{
		CPen pen(PS_SOLID, 2, RGB(255, 220, 0));
		CPen* oldPen = pDC->SelectObject(&pen);
		CBrush* oldBrush = static_cast<CBrush*>(pDC->SelectStockObject(NULL_BRUSH));
		pDC->Rectangle(selection);
		pDC->SelectObject(oldBrush);
		pDC->SelectObject(oldPen);
	}
}

BOOL CAIMLImageExtractorView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

bool CAIMLImageExtractorView::ClientToFramePoint(CPoint point, CPoint& framePoint) const
{
	const CAIMLImageExtractorDoc* pDoc = GetDocument();
	if (pDoc == nullptr)
		return false;
	const cv::Mat& frame = pDoc->GetCurrentFrame();

	CRect clientRect, drawRect;
	GetClientRect(&clientRect);
	double scale = 1.0;
	if (!GetFrameLayout(clientRect, frame, drawRect, scale))
		return false;

	int x = static_cast<int>((point.x - drawRect.left) / scale);
	int y = static_cast<int>((point.y - drawRect.top) / scale);
	x = x < 0 ? 0 : (x > frame.cols ? frame.cols : x);
	y = y < 0 ? 0 : (y > frame.rows ? frame.rows : y);
	framePoint.SetPoint(x, y);
	return true;
}

void CAIMLImageExtractorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CAIMLImageExtractorDoc* pDoc = GetDocument();
	CPoint framePoint;
	SetFocus();
	if (pDoc != nullptr && ClientToFramePoint(point, framePoint))
	{
		m_selecting = true;
		m_hasSelection = false;
		m_dragStart = m_dragCurrent = point;
		SetCapture();
		Invalidate(FALSE);
	}
	CView::OnLButtonDown(nFlags, point);
}

void CAIMLImageExtractorView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_selecting)
	{
		m_dragCurrent = point;
		Invalidate(FALSE);
	}
	CView::OnMouseMove(nFlags, point);
}

void CAIMLImageExtractorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_selecting)
	{
		m_selecting = false;
		::ReleaseCapture();
		m_dragCurrent = point;

		CAIMLImageExtractorDoc* pDoc = GetDocument();
		CPoint start, end;
		if (pDoc != nullptr && ClientToFramePoint(m_dragStart, start) && ClientToFramePoint(m_dragCurrent, end))
		{
			const cv::Rect region(
				cv::Point(start.x < end.x ? start.x : end.x, start.y < end.y ? start.y : end.y),
				cv::Point(start.x < end.x ? end.x : start.x, start.y < end.y ? end.y : start.y));
			// A plain click (or a tiny drag) is not a selection.
			if (region.width >= kMinSelectionSize && region.height >= kMinSelectionSize)
			{
				m_selectionRect = region;
				m_selectionFrame = pDoc->GetCurrentFrameIndex();
				m_hasSelection = true;
				// Repaint first so the rectangle stays visible while the species dialog is open.
				Invalidate(FALSE);
				UpdateWindow();
				if (!pDoc->ExtractRegion(region))
					m_hasSelection = false;
			}
		}
		Invalidate(FALSE);
	}
	CView::OnLButtonUp(nFlags, point);
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
