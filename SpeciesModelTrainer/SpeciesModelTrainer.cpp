
#include "SpeciesModelTrainer.h"
#include "TrainerDlg.h"

CSpeciesModelTrainerApp theApp;

BOOL CSpeciesModelTrainerApp::InitInstance()
{
	CWinApp::InitInstance();

	CTrainerDlg dialog;
	m_pMainWnd = &dialog;
	dialog.DoModal();
	return FALSE;
}
