
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

void CAIMLImageExtractorApp::OnFileExportDataset()
{
	if (!m_speciesDb.IsOpen() || m_speciesDb.GetImageCount() == 0)
	{
		AfxMessageBox(_T("There are no stored images to export."), MB_ICONINFORMATION);
		return;
	}
	CFolderPickerDialog picker(nullptr, 0, AfxGetMainWnd());
	if (picker.DoModal() != IDOK)
		return;
	const bool includeUnknown = AfxMessageBox(
		_T("Include images labelled Unknown in the export?"), MB_YESNO | MB_ICONQUESTION) == IDYES;

	CWaitCursor wait;
	CString error;
	const int count = ExportDataset(m_speciesDb, picker.GetPathName(), includeUnknown, error);
	CString message;
	if (count < 0)
		message = _T("Export failed: ") + error;
	else
		message.Format(_T("Exported %d image(s) to %s (class folders plus manifest.csv)"), count, static_cast<LPCTSTR>(picker.GetPathName()));
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




