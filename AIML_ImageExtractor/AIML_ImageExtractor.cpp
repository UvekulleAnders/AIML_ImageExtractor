// AIML_ImageExtractor.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "AIML_ImageExtractor.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "AIML_ImageExtractorDoc.h"
#include "AIML_ImageExtractorView.h"
#include "DatasetExporter.h"
#include "SpeciesExportChecklistDlg.h"
#include "EditDatabaseDlg.h"
#include "BatchImageList.h"
#include <filesystem>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAIMLImageExtractorApp

BEGIN_MESSAGE_MAP(CAIMLImageExtractorApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CAIMLImageExtractorApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CAIMLImageExtractorApp::OnFileOpen)
	ON_COMMAND(ID_FILE_EXPORT_DATASET, &CAIMLImageExtractorApp::OnFileExportDataset)
	ON_COMMAND(ID_FILE_EDIT_DATABASE, &CAIMLImageExtractorApp::OnFileEditDatabase)
	ON_COMMAND(ID_FILE_PROCESS_IMAGES_FROM_FILE, &CAIMLImageExtractorApp::OnFileProcessImagesFromFile)
	ON_COMMAND(ID_FILE_STOP_PROCESS_IMAGES, &CAIMLImageExtractorApp::OnFileStopProcessImages)
	ON_UPDATE_COMMAND_UI(ID_FILE_STOP_PROCESS_IMAGES, &CAIMLImageExtractorApp::OnUpdateFileStopProcessImages)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CAIMLImageExtractorApp construction

CAIMLImageExtractorApp::CAIMLImageExtractorApp() noexcept
{
	m_bHiColorIcons = TRUE;


	m_nAppLook = 0;
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("AIMLImageExtractor.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CAIMLImageExtractorApp object

CAIMLImageExtractorApp theApp;


// CAIMLImageExtractorApp initialization

void CAIMLImageExtractorApp::OnFileOpen()
{
	static const TCHAR filter[] =
		_T("Video and Image Files (*.avi;*.mp4;*.mkv;*.mov;*.wmv;*.flv;*.mpeg;*.mpg;*.m4v;*.webm;*.h264;*.264;*.jpg;*.jpeg)\0")
		_T("*.avi;*.mp4;*.mkv;*.mov;*.wmv;*.flv;*.mpeg;*.mpg;*.m4v;*.webm;*.h264;*.264;*.jpg;*.jpeg\0")
		_T("JPEG Images (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0")
		_T("All Files (*.*)\0*.*\0\0");
	CFileDialog dialog(TRUE, nullptr, nullptr,
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, filter);
	if (dialog.DoModal() == IDOK)
		OpenDocumentFile(dialog.GetPathName());
}

void CAIMLImageExtractorApp::OnFileProcessImagesFromFile()
{
	CFileDialog dialog(TRUE, _T("txt"), nullptr,
		OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
		_T("Image list (*.txt)|*.txt|All Files (*.*)|*.*||"));
	dialog.m_ofn.lpstrTitle = _T("Select a text file listing the images to process");
	if (dialog.DoModal() != IDOK)
		return;

	const std::wstring listFile(dialog.GetPathName().GetString());
	std::vector<std::wstring> images;
	if (!LoadImageList(listFile, images))
	{
		AfxMessageBox(_T("The image list could not be read."), MB_ICONERROR);
		return;
	}
	if (images.empty())
	{
		AfxMessageBox(_T("The image list contains no image paths."), MB_ICONINFORMATION);
		return;
	}

	size_t start = LoadBatchProgress(listFile);
	if (start > 0)
	{
		CString question;
		if (start >= images.size())
			question.Format(_T("All %d images in this list were processed earlier.\n\nStart over from the first image?"),
				static_cast<int>(images.size()));
		else
			question.Format(_T("%d of %d images were already processed.\n\nYes = resume with image %d\nNo = start over from the first image"),
				static_cast<int>(start), static_cast<int>(images.size()), static_cast<int>(start) + 1);
		const UINT type = start >= images.size() ? MB_YESNO : MB_YESNOCANCEL;
		const int answer = AfxMessageBox(question, type | MB_ICONQUESTION);
		if (answer == IDCANCEL || (start >= images.size() && answer != IDYES))
			return;
		if (start >= images.size() || answer == IDNO)
			start = 0;
	}

	m_batchListFile = listFile;
	m_batchImages = std::move(images);
	m_batchNext = start;
	m_batchCurrent = start;
	m_batchDoc = nullptr;
	m_batchActive = true;
	OpenNextBatchImage();
}

void CAIMLImageExtractorApp::OnFileStopProcessImages()
{
	if (m_batchActive)
		FinishBatch(_T("ProcessImagesFromFile stopped; run it again on the same list to resume."));
}

void CAIMLImageExtractorApp::OnUpdateFileStopProcessImages(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_batchActive);
}

void CAIMLImageExtractorApp::FinishBatch(const CString& message)
{
	m_batchActive = false;
	m_batchDoc = nullptr;
	if (CFrameWnd* frame = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd()))
		frame->SetMessageText(message);
	if (CWnd* main = AfxGetMainWnd())
		main->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void CAIMLImageExtractorApp::OnDocumentClosing(const CDocument* doc)
{
	if (!m_batchActive || doc != m_batchDoc)
		return;
	m_batchDoc = nullptr;
	SaveBatchProgress(m_batchListFile, m_batchCurrent + 1);
	// Open the next image only after this document is fully closed.
	if (CWnd* main = AfxGetMainWnd())
		main->PostMessage(WM_BATCH_OPEN_NEXT);
}

void CAIMLImageExtractorApp::OpenNextBatchImage()
{
	if (!m_batchActive || m_batchDoc != nullptr)
		return;

	int skipped = 0;
	while (m_batchNext < m_batchImages.size())
	{
		const size_t index = m_batchNext++;
		const CString path(m_batchImages[index].c_str());
		const bool isJpeg = path.Right(4).CompareNoCase(_T(".jpg")) == 0 || path.Right(5).CompareNoCase(_T(".jpeg")) == 0;
		if (!isJpeg || ::GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES)
		{
			++skipped;
			continue;
		}
		CDocument* doc = OpenDocumentFile(path);
		if (doc == nullptr)
		{
			++skipped;
			continue;
		}
		m_batchCurrent = index;
		m_batchDoc = doc;
		if (CAIMLImageExtractorDoc* imageDoc = DYNAMIC_DOWNCAST(CAIMLImageExtractorDoc, doc))
			imageDoc->DetectObjects();
		CString message;
		message.Format(_T("ProcessImagesFromFile: image %d / %d"), static_cast<int>(index) + 1, static_cast<int>(m_batchImages.size()));
		if (skipped > 0)
			message.AppendFormat(_T(" (%d unreadable or non-JPEG entries skipped)"), skipped);
		if (CFrameWnd* frame = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd()))
			frame->SetMessageText(message);
		doc->UpdateAllViews(nullptr);
		return;
	}

	SaveBatchProgress(m_batchListFile, m_batchImages.size());
	CString done;
	done.Format(_T("ProcessImagesFromFile finished: all %d images in the list were processed."), static_cast<int>(m_batchImages.size()));
	FinishBatch(done);
	AfxMessageBox(done, MB_ICONINFORMATION);
}

void CAIMLImageExtractorApp::OnFileEditDatabase()
{
	if (!m_speciesDb.IsOpen() || m_speciesDb.GetImageCount() == 0)
	{
		AfxMessageBox(_T("There are no stored images to edit."), MB_ICONINFORMATION);
		return;
	}
	CEditDatabaseDlg dialog(m_speciesDb, AfxGetMainWnd());
	dialog.DoModal();
}

void CAIMLImageExtractorApp::OnFileExportDataset()
{
	if (!m_speciesDb.IsOpen() || m_speciesDb.GetImageCount() == 0)
	{
		AfxMessageBox(_T("There are no stored images to export."), MB_ICONINFORMATION);
		return;
	}

	const std::vector<SpeciesEntryCount> species = m_speciesDb.GetSpeciesWithEntryCounts();
	CSpeciesExportChecklistDlg checklist(species, AfxGetMainWnd());
	if (checklist.DoModal() != IDOK)
		return;

	CFolderPickerDialog picker(nullptr, 0, AfxGetMainWnd());
	if (picker.DoModal() != IDOK)
		return;
	const bool trainingReady = AfxMessageBox(
		_T("Only export species with images from at least two different source files?\n\n")
		_T("Training needs this for its validation split; other species would make training fail."),
		MB_YESNO | MB_ICONQUESTION) == IDYES;

	CWaitCursor wait;
	CString error;
	std::vector<CString> skipped;
	const int count = ExportDataset(m_speciesDb, picker.GetPathName(), checklist.GetSelectedSpeciesIds(), error, trainingReady ? 2 : 0, &skipped);
	CString message;
	if (count < 0)
		message = _T("Export failed: ") + error;
	else
		message.Format(_T("Exported %d image(s) to %s (class folders plus manifest.csv)"), count, static_cast<LPCTSTR>(picker.GetPathName()));
	if (count >= 0 && !skipped.empty())
	{
		message += _T("\n\nSkipped (fewer than two source files): ");
		for (size_t i = 0; i < skipped.size(); ++i)
			message += (i ? _T(", ") : _T("")) + skipped[i];
	}
	AfxMessageBox(message, count < 0 ? MB_ICONERROR : MB_ICONINFORMATION);
}

BOOL CAIMLImageExtractorApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Open (or create) the species database next to the executable.
	TCHAR modulePath[MAX_PATH] = {};
	::GetModuleFileName(nullptr, modulePath, MAX_PATH);
	CString databasePath(modulePath);
	databasePath = databasePath.Left(databasePath.ReverseFind(_T('\\')) + 1) + _T("speciesImages.db");
	if (!m_speciesDb.Open(databasePath))
	{
		CString message;
		message.Format(_T("Could not open the species database:\n%s\n\n%s"),
			static_cast<LPCTSTR>(databasePath), static_cast<LPCTSTR>(m_speciesDb.GetLastError()));
		AfxMessageBox(message, MB_ICONWARNING);
	}

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_AIMLImageExtractorTYPE,
		RUNTIME_CLASS(CAIMLImageExtractorDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CAIMLImageExtractorView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	pMainFrame->DragAcceptFiles();

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CAIMLImageExtractorApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	m_speciesDb.Close();
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CAIMLImageExtractorApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CAIMLImageExtractorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CAIMLImageExtractorApp customization load/save methods

void CAIMLImageExtractorApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CAIMLImageExtractorApp::LoadCustomState()
{
}

void CAIMLImageExtractorApp::SaveCustomState()
{
}

// CAIMLImageExtractorApp message handlers




