
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

	return CView::PreCreateWindow(cs);
}

// CAIMLImageExtractorView drawing

void CAIMLImageExtractorView::OnDraw(CDC* /*pDC*/)
{
	CAIMLImageExtractorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
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
