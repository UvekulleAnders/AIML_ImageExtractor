
#include "TrainerDlg.h"
#include "Resource.h"
#include <afxdlgs.h>
#include <algorithm>
#include <memory>
#include <string>

namespace
{
	enum { WM_TRAINER_OUTPUT = WM_APP + 1, WM_TRAINER_FINISHED = WM_APP + 2 };

	struct TrainingJob
	{
		CTrainerDlg* dialog;
		CString dataset;
		CString output;
		CString script;
		CString workingDirectory;
		UINT epochs;
		UINT batchSize;
	};

	CString QuoteArgument(const CString& value)
	{
		return _T("\"") + value + _T("\"");
	}

	CString DecodeUtf8(const char* bytes, DWORD length)
	{
		if (length == 0)
			return CString();
		const int wideLength = MultiByteToWideChar(CP_UTF8, 0, bytes, static_cast<int>(length), nullptr, 0);
		CString text;
		LPWSTR buffer = text.GetBuffer(wideLength);
		MultiByteToWideChar(CP_UTF8, 0, bytes, static_cast<int>(length), buffer, wideLength);
		text.ReleaseBuffer(wideLength);
		return text;
	}

	void PostOutput(CTrainerDlg* dialog, const CString& text)
	{
		CString* payload = new CString(text);
		if (!dialog->PostMessage(WM_TRAINER_OUTPUT, 0, reinterpret_cast<LPARAM>(payload)))
			delete payload;
	}
}

BEGIN_MESSAGE_MAP(CTrainerDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BROWSE_DATASET, &CTrainerDlg::OnBrowseDataset)
	ON_BN_CLICKED(IDC_BROWSE_OUTPUT, &CTrainerDlg::OnBrowseOutput)
	ON_BN_CLICKED(IDC_START_TRAINING, &CTrainerDlg::OnStartTraining)
	ON_BN_CLICKED(IDC_STOP_TRAINING, &CTrainerDlg::OnStopTraining)
	ON_MESSAGE(WM_TRAINER_OUTPUT, &CTrainerDlg::OnWorkerOutput)
	ON_MESSAGE(WM_TRAINER_FINISHED, &CTrainerDlg::OnWorkerFinished)
END_MESSAGE_MAP()

CTrainerDlg::CTrainerDlg() : CDialogEx(IDD_TRAINER_DIALOG)
{
}

CTrainerDlg::~CTrainerDlg()
{
}

BOOL CTrainerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetDlgItemText(IDC_EPOCHS, _T("10"));
	SetDlgItemText(IDC_BATCH_SIZE, _T("16"));
	SetDlgItemText(IDC_OUTPUT_PATH, _T("species_model.onnx"));
	SetDlgItemText(IDC_STATUS, _T("Ready. Training requires Python, PyTorch, torchvision, Pillow, and ONNX."));
	return TRUE;
}

void CTrainerDlg::OnBrowseDataset()
{
	CFolderPickerDialog dialog(nullptr, OFN_PATHMUSTEXIST, this);
	if (dialog.DoModal() == IDOK)
		SetDlgItemText(IDC_DATASET_PATH, dialog.GetPathName());
}

void CTrainerDlg::OnBrowseOutput()
{
	CString filter = _T("ONNX model (*.onnx)|*.onnx|All files (*.*)|*.*||");
	CFileDialog dialog(FALSE, _T("onnx"), _T("species_model.onnx"), OFN_OVERWRITEPROMPT, filter, this);
	if (dialog.DoModal() == IDOK)
		SetDlgItemText(IDC_OUTPUT_PATH, dialog.GetPathName());
}

void CTrainerDlg::OnStartTraining()
{
	if (m_running)
		return;

	CString dataset;
	CString output;
	GetDlgItemText(IDC_DATASET_PATH, dataset);
	GetDlgItemText(IDC_OUTPUT_PATH, output);
	BOOL validEpochs = FALSE;
	BOOL validBatchSize = FALSE;
	const UINT epochs = GetDlgItemInt(IDC_EPOCHS, &validEpochs, FALSE);
	const UINT batchSize = GetDlgItemInt(IDC_BATCH_SIZE, &validBatchSize, FALSE);

	if (dataset.IsEmpty() || output.IsEmpty())
	{
		AfxMessageBox(_T("Choose a dataset export and an output model path."), MB_ICONWARNING);
		return;
	}
	if (!validEpochs || epochs == 0 || !validBatchSize || batchSize == 0)
	{
		AfxMessageBox(_T("Epochs and batch size must be positive integers."), MB_ICONWARNING);
		return;
	}
	if (::GetFileAttributes(dataset + _T("\\manifest.csv")) == INVALID_FILE_ATTRIBUTES)
	{
		AfxMessageBox(_T("The selected folder does not contain manifest.csv."), MB_ICONWARNING);
		return;
	}

	TCHAR modulePath[MAX_PATH] = {};
	::GetModuleFileName(nullptr, modulePath, MAX_PATH);
	CString workingDirectory(modulePath);
	const int separator = workingDirectory.ReverseFind(_T('\\'));
	workingDirectory = separator >= 0 ? workingDirectory.Left(separator) : CString(_T("."));
	CString script = workingDirectory + _T("\\train_species.py");
	if (::GetFileAttributes(script) == INVALID_FILE_ATTRIBUTES)
	{
		AfxMessageBox(_T("train_species.py was not found beside the trainer executable."), MB_ICONERROR);
		return;
	}

	TrainingJob* job = new TrainingJob{ this, dataset, output, script, workingDirectory, epochs, batchSize };
	InterlockedExchange(&m_cancelRequested, 0);
	m_workerThread = AfxBeginThread(&CTrainerDlg::TrainingThread, job, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_workerThread == nullptr)
	{
		delete job;
		AfxMessageBox(_T("Could not start the training worker."), MB_ICONERROR);
		return;
	}
	m_workerThread->m_bAutoDelete = FALSE;
	SetRunning(true);
	AppendLog(_T("Starting Python training worker...\r\n"));
	m_workerThread->ResumeThread();
}

void CTrainerDlg::OnStopTraining()
{
	if (m_running)
	{
		InterlockedExchange(&m_cancelRequested, 1);
		SetDlgItemText(IDC_STATUS, _T("Stopping training..."));
		GetDlgItem(IDC_STOP_TRAINING)->EnableWindow(FALSE);
	}
}

void CTrainerDlg::OnCancel()
{
	if (m_workerThread != nullptr)
	{
		InterlockedExchange(&m_cancelRequested, 1);
		::WaitForSingleObject(m_workerThread->m_hThread, INFINITE);
		delete m_workerThread;
		m_workerThread = nullptr;
	}
	CDialogEx::OnCancel();
}

void CTrainerDlg::SetRunning(bool running)
{
	m_running = running;
	GetDlgItem(IDC_DATASET_PATH)->EnableWindow(!running);
	GetDlgItem(IDC_BROWSE_DATASET)->EnableWindow(!running);
	GetDlgItem(IDC_OUTPUT_PATH)->EnableWindow(!running);
	GetDlgItem(IDC_BROWSE_OUTPUT)->EnableWindow(!running);
	GetDlgItem(IDC_EPOCHS)->EnableWindow(!running);
	GetDlgItem(IDC_BATCH_SIZE)->EnableWindow(!running);
	GetDlgItem(IDC_START_TRAINING)->EnableWindow(!running);
	GetDlgItem(IDC_STOP_TRAINING)->EnableWindow(running);
}

void CTrainerDlg::AppendLog(const CString& text)
{
	CEdit* log = static_cast<CEdit*>(GetDlgItem(IDC_LOG));
	const int length = log->GetWindowTextLength();
	log->SetSel(length, length);
	log->ReplaceSel(text);
}

LRESULT CTrainerDlg::OnWorkerOutput(WPARAM, LPARAM lParam)
{
	std::unique_ptr<CString> text(reinterpret_cast<CString*>(lParam));
	if (text)
		AppendLog(*text);
	return 0;
}

LRESULT CTrainerDlg::OnWorkerFinished(WPARAM wParam, LPARAM)
{
	if (m_workerThread != nullptr)
	{
		::WaitForSingleObject(m_workerThread->m_hThread, INFINITE);
		delete m_workerThread;
		m_workerThread = nullptr;
	}
	SetRunning(false);
	const DWORD exitCode = static_cast<DWORD>(wParam);
	if (exitCode == 0)
		SetDlgItemText(IDC_STATUS, _T("Training complete."));
	else if (InterlockedCompareExchange(&m_cancelRequested, 0, 0) != 0)
		SetDlgItemText(IDC_STATUS, _T("Training stopped."));
	else
		SetDlgItemText(IDC_STATUS, _T("Training failed. See the log for details."));
	return 0;
}

UINT CTrainerDlg::TrainingThread(LPVOID parameter)
{
	std::unique_ptr<TrainingJob> job(static_cast<TrainingJob*>(parameter));
	SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
	HANDLE readPipe = nullptr;
	HANDLE writePipe = nullptr;
	DWORD exitCode = 1;
	if (!::CreatePipe(&readPipe, &writePipe, &security, 0))
	{
		PostOutput(job->dialog, _T("ERROR: Could not create the Python output pipe.\r\n"));
		job->dialog->PostMessage(WM_TRAINER_FINISHED, exitCode, 0);
		return exitCode;
	}
	::SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	startup.wShowWindow = SW_HIDE;
	startup.hStdOutput = writePipe;
	startup.hStdError = writePipe;
	startup.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
	PROCESS_INFORMATION process = {};

	CString command = _T("python -X utf8 -u ") + QuoteArgument(job->script) +
		_T(" --dataset ") + QuoteArgument(job->dataset) +
		_T(" --output ") + QuoteArgument(job->output);
	CString number;
	number.Format(_T(" --epochs %u --batch-size %u"), job->epochs, job->batchSize);
	command += number;
	const BOOL created = ::CreateProcess(nullptr, command.GetBuffer(), nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW, nullptr, job->workingDirectory, &startup, &process);
	command.ReleaseBuffer();
	::CloseHandle(writePipe);

	if (!created)
	{
		const DWORD error = ::GetLastError();
		CString message;
		message.Format(_T("ERROR: Could not start Python (Windows error %lu). Check Python is on PATH.\r\n"), error);
		PostOutput(job->dialog, message);
	}
	else
	{
		bool processExited = false;
		while (!processExited)
		{
			if (InterlockedCompareExchange(&job->dialog->m_cancelRequested, 0, 0) != 0)
				::TerminateProcess(process.hProcess, 1);

			DWORD available = 0;
			if (::PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr) && available > 0)
			{
				char buffer[2048];
				DWORD bytesRead = 0;
				if (::ReadFile(readPipe, buffer, std::min<DWORD>(available, static_cast<DWORD>(sizeof(buffer))), &bytesRead, nullptr) && bytesRead > 0)
					PostOutput(job->dialog, DecodeUtf8(buffer, bytesRead));
			}

			processExited = ::WaitForSingleObject(process.hProcess, 50) == WAIT_OBJECT_0;
		}

		for (;;)
		{
			DWORD available = 0;
			if (!::PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr) || available == 0)
				break;
			char buffer[2048];
			DWORD bytesRead = 0;
			if (!::ReadFile(readPipe, buffer, std::min<DWORD>(available, static_cast<DWORD>(sizeof(buffer))), &bytesRead, nullptr) || bytesRead == 0)
				break;
			PostOutput(job->dialog, DecodeUtf8(buffer, bytesRead));
		}
		::GetExitCodeProcess(process.hProcess, &exitCode);
		::CloseHandle(process.hThread);
		::CloseHandle(process.hProcess);
	}

	::CloseHandle(readPipe);
	job->dialog->PostMessage(WM_TRAINER_FINISHED, exitCode, 0);
	return exitCode;
}
