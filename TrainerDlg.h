#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include "Resource.h"

class CTrainerDlg : public CDialogEx
{
public:
	enum { IDD = IDD_TRAINER_DIALOG };
	CTrainerDlg();
	virtual ~CTrainerDlg();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnBrowseDataset();
	afx_msg void OnBrowseOutput();
	afx_msg void OnStartTraining();
	afx_msg void OnStopTraining();
	afx_msg LRESULT OnWorkerOutput(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWorkerFinished(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	static UINT TrainingThread(LPVOID parameter);
	void SetRunning(bool running);
	void AppendLog(const CString& text);

	CWinThread* m_workerThread = nullptr;
	volatile LONG m_cancelRequested = 0;
	bool m_running = false;
};
