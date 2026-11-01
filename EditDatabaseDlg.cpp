// EditDatabaseDlg.cpp : implementation of the CEditDatabaseDlg class
//

#include "pch.h"
#include "EditDatabaseDlg.h"
#include "AssignSpeciesDlg.h"

namespace
{
	constexpr UINT IDC_IMAGE_LIST = 2101;
	constexpr UINT IDC_EDIT_SWEDISH = 2102;
	constexpr UINT IDC_EDIT_LATIN = 2103;
	constexpr UINT IDC_EDIT_ENGLISH = 2104;
	constexpr UINT IDC_SAVE_SPECIES = 2105;
	constexpr UINT IDC_CHANGE_SPECIES = 2106;
	constexpr UINT IDC_EDIT_STATUS = 2107;
	constexpr UINT IDC_AGE_COMBO = 2108;

	constexpr short kDialogWidth = 360;
	constexpr short kDialogHeight = 244;

	void AppendWord(std::vector<BYTE>& buffer, WORD value)
	{
		buffer.push_back(static_cast<BYTE>(value & 0xff));
		buffer.push_back(static_cast<BYTE>(value >> 8));
	}

	void AppendDword(std::vector<BYTE>& buffer, DWORD value)
	{
		AppendWord(buffer, static_cast<WORD>(value & 0xffff));
		AppendWord(buffer, static_cast<WORD>(value >> 16));
	}

	void AppendString(std::vector<BYTE>& buffer, const wchar_t* text)
	{
		do
		{
			AppendWord(buffer, static_cast<WORD>(*text));
		} while (*text++ != L'\0');
	}

	CString FileNameOnly(const CString& path)
	{
		const int separator = path.ReverseFind(_T('\\'));
		return separator >= 0 ? path.Mid(separator + 1) : path;
	}
}

BEGIN_MESSAGE_MAP(CEditDatabaseDlg, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IMAGE_LIST, &CEditDatabaseDlg::OnImageSelected)
	ON_BN_CLICKED(IDC_SAVE_SPECIES, &CEditDatabaseDlg::OnSaveSpecies)
	ON_BN_CLICKED(IDC_CHANGE_SPECIES, &CEditDatabaseDlg::OnChangeSpecies)
	ON_CBN_SELCHANGE(IDC_AGE_COMBO, &CEditDatabaseDlg::OnAgeChanged)
END_MESSAGE_MAP()

CEditDatabaseDlg::CEditDatabaseDlg(CSpeciesDatabase& database, CWnd* parent)
	: CDialog(), m_database(database)
{
	m_pParentWnd = parent;
}

INT_PTR CEditDatabaseDlg::DoModal()
{
	std::vector<BYTE> tmpl;
	AppendDword(tmpl, DS_MODALFRAME | DS_CENTER | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU);
	AppendDword(tmpl, 0);
	AppendWord(tmpl, 0);
	AppendWord(tmpl, 0);
	AppendWord(tmpl, 0);
	AppendWord(tmpl, static_cast<WORD>(kDialogWidth));
	AppendWord(tmpl, static_cast<WORD>(kDialogHeight));
	AppendWord(tmpl, 0);
	AppendWord(tmpl, 0);
	AppendString(tmpl, L"Edit database");
	AppendWord(tmpl, 9);
	AppendString(tmpl, L"Segoe UI");

	HGLOBAL memory = ::GlobalAlloc(GMEM_FIXED, tmpl.size()); // MFC treats the handle as a pointer
	if (memory == nullptr)
		return -1;
	memcpy(memory, tmpl.data(), tmpl.size());

	if (!InitModalIndirect(memory, m_pParentWnd))
	{
		::GlobalFree(memory);
		return -1;
	}
	const INT_PTR result = CDialog::DoModal();
	::GlobalFree(memory);
	return result;
}

void CEditDatabaseDlg::PlaceControl(CWnd& control, int x, int y, int cx, int cy)
{
	CRect rect(x, y, x + cx, y + cy);
	MapDialogRect(&rect);
	control.MoveWindow(rect);
	control.SetFont(GetFont());
}

BOOL CEditDatabaseDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const DWORD child = WS_CHILD | WS_VISIBLE;
	const DWORD tab = WS_TABSTOP;
	const CRect none(0, 0, 10, 10);

	m_list.CreateEx(WS_EX_CLIENTEDGE, child | tab | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, none, this, IDC_IMAGE_LIST);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_speciesGroup.Create(_T("Species of the selected image"), child | SS_LEFTNOWORDWRAP, none, this);
	m_swedishLabel.Create(_T("Swedish name:"), child, none, this);
	m_swedishEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_EDIT_SWEDISH);
	m_latinLabel.Create(_T("Latin name:"), child, none, this);
	m_latinEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_EDIT_LATIN);
	m_englishLabel.Create(_T("English name:"), child, none, this);
	m_englishEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_EDIT_ENGLISH);
	m_saveButton.Create(_T("Save names"), child | tab | BS_PUSHBUTTON, none, this, IDC_SAVE_SPECIES);
	m_changeButton.Create(_T("Link image to another species..."), child | tab | BS_PUSHBUTTON, none, this, IDC_CHANGE_SPECIES);
	m_ageLabel.Create(_T("Age of image:"), child, none, this);
	m_ageCombo.Create(child | tab | WS_VSCROLL | CBS_DROPDOWNLIST, none, this, IDC_AGE_COMBO);
	m_status.Create(_T(""), child | SS_LEFTNOWORDWRAP, none, this, IDC_EDIT_STATUS);
	m_closeButton.Create(_T("Close"), child | tab | BS_DEFPUSHBUTTON, none, this, IDOK);

	PlaceControl(m_list, 7, 7, 346, 120);
	PlaceControl(m_speciesGroup, 7, 134, 346, 10);
	PlaceControl(m_swedishLabel, 14, 150, 60, 10);
	PlaceControl(m_swedishEdit, 80, 148, 180, 13);
	PlaceControl(m_latinLabel, 14, 166, 60, 10);
	PlaceControl(m_latinEdit, 80, 164, 180, 13);
	PlaceControl(m_englishLabel, 14, 182, 60, 10);
	PlaceControl(m_englishEdit, 80, 180, 180, 13);
	PlaceControl(m_saveButton, 268, 148, 85, 14);
	PlaceControl(m_changeButton, 220, 198, 133, 14);
	PlaceControl(m_status, 14, 200, 200, 10);
	PlaceControl(m_ageLabel, 14, 226, 60, 10);
	PlaceControl(m_ageCombo, 80, 224, 180, 120);
	PlaceControl(m_closeButton, 303, 223, 50, 14);

	CRect listRect;
	m_list.GetClientRect(&listRect);
	const int width = listRect.Width() - ::GetSystemMetrics(SM_CXVSCROLL);
	m_list.InsertColumn(0, _T("Image"), LVCFMT_LEFT, width / 10);
	m_list.InsertColumn(1, _T("Species"), LVCFMT_LEFT, width * 3 / 10);
	m_list.InsertColumn(2, _T("Source file"), LVCFMT_LEFT, width * 3 / 10);
	m_list.InsertColumn(3, _T("Frame"), LVCFMT_LEFT, width / 10);
	m_list.InsertColumn(4, _T("Age"), LVCFMT_LEFT, width / 10);

	int ageCount = 0;
	const AgeCode* ages = GetAgeCodes(ageCount);
	for (int i = 0; i < ageCount; ++i)
		m_ageCombo.AddString(CString(ages[i].code) + _T(" - ") + ages[i].name);

	Reload(0);
	m_list.SetFocus();
	return FALSE;
}

void CEditDatabaseDlg::OnOK()
{
	CDialog::OnOK();
}

const SpeciesRecord* CEditDatabaseDlg::FindSpecies(long long id) const
{
	for (const SpeciesRecord& species : m_species)
	{
		if (species.id == id)
			return &species;
	}
	return nullptr;
}

long long CEditDatabaseDlg::SelectedImageId() const
{
	const int index = m_list.GetNextItem(-1, LVNI_SELECTED);
	return index < 0 ? 0 : static_cast<long long>(m_list.GetItemData(index));
}

void CEditDatabaseDlg::Reload(long long selectImageId)
{
	m_species = m_database.GetSpecies();
	m_images = m_database.GetImages();

	m_list.DeleteAllItems();
	int selectIndex = m_images.empty() ? -1 : 0;
	for (size_t i = 0; i < m_images.size(); ++i)
	{
		const ImageRecord& image = m_images[i];
		const SpeciesRecord* species = FindSpecies(image.speciesId);
		CString idText, frameText;
		idText.Format(_T("%lld"), image.id);
		frameText.Format(_T("%lld"), image.frameIndex);
		const int row = m_list.InsertItem(static_cast<int>(i), idText);
		m_list.SetItemData(row, static_cast<DWORD_PTR>(image.id));
		m_list.SetItemText(row, 1, species != nullptr ? species->DisplayName() : CString(_T("Unknown")));
		m_list.SetItemText(row, 2, FileNameOnly(image.sourceFile));
		m_list.SetItemText(row, 3, frameText);
		m_list.SetItemText(row, 4, image.ageCode);
		if (image.id == selectImageId)
			selectIndex = row;
	}
	if (selectIndex >= 0)
	{
		m_list.SetItemState(selectIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_list.EnsureVisible(selectIndex, FALSE);
	}
	UpdateSpeciesFields();
}

void CEditDatabaseDlg::UpdateSpeciesFields()
{
	const long long imageId = SelectedImageId();
	const ImageRecord* image = nullptr;
	for (const ImageRecord& candidate : m_images)
	{
		if (candidate.id == imageId)
			image = &candidate;
	}
	const SpeciesRecord* species = image != nullptr ? FindSpecies(image->speciesId) : nullptr;

	m_swedishEdit.SetWindowText(species != nullptr ? species->swedishName : CString());
	m_latinEdit.SetWindowText(species != nullptr ? species->latinName : CString());
	m_englishEdit.SetWindowText(species != nullptr ? species->englishName : CString());

	const BOOL editable = species != nullptr && species->id != kUnknownSpeciesId;
	m_swedishEdit.EnableWindow(editable);
	m_latinEdit.EnableWindow(editable);
	m_englishEdit.EnableWindow(editable);
	m_saveButton.EnableWindow(editable);
	m_changeButton.EnableWindow(image != nullptr);
	m_ageCombo.EnableWindow(image != nullptr);
	m_ageCombo.SetCurSel(-1);
	if (image != nullptr)
	{
		int ageCount = 0;
		const AgeCode* ages = GetAgeCodes(ageCount);
		for (int i = 0; i < ageCount; ++i)
		{
			if (image->ageCode == ages[i].code)
				m_ageCombo.SetCurSel(i);
		}
	}
	if (image != nullptr && !editable)
		m_status.SetWindowText(_T("Unknown has no names to edit; link the image to a species instead."));
	else
		m_status.SetWindowText(_T(""));
}

void CEditDatabaseDlg::OnImageSelected(NMHDR* header, LRESULT* result)
{
	const NMLISTVIEW* view = reinterpret_cast<NMLISTVIEW*>(header);
	if ((view->uChanged & LVIF_STATE) && (view->uNewState & LVIS_SELECTED))
		UpdateSpeciesFields();
	*result = 0;
}

void CEditDatabaseDlg::OnSaveSpecies()
{
	const long long imageId = SelectedImageId();
	const ImageRecord* image = nullptr;
	for (const ImageRecord& candidate : m_images)
	{
		if (candidate.id == imageId)
			image = &candidate;
	}
	if (image == nullptr)
		return;

	SpeciesRecord species;
	species.id = image->speciesId;
	m_swedishEdit.GetWindowText(species.swedishName);
	m_latinEdit.GetWindowText(species.latinName);
	m_englishEdit.GetWindowText(species.englishName);
	species.swedishName.Trim();
	species.latinName.Trim();
	species.englishName.Trim();
	if (species.latinName.IsEmpty())
	{
		m_status.SetWindowText(_T("The Latin name is compulsory."));
		m_latinEdit.SetFocus();
		return;
	}
	if (!m_database.UpdateSpecies(species))
	{
		m_status.SetWindowText(_T("Could not save (does another species use that Latin name?)."));
		return;
	}
	Reload(imageId);
	m_status.SetWindowText(_T("Names saved."));
}

void CEditDatabaseDlg::OnAgeChanged()
{
	const long long imageId = SelectedImageId();
	const int index = m_ageCombo.GetCurSel();
	int ageCount = 0;
	const AgeCode* ages = GetAgeCodes(ageCount);
	if (imageId == 0 || index < 0 || index >= ageCount)
		return;
	if (!m_database.SetImageAge(imageId, ages[index].code))
	{
		m_status.SetWindowText(_T("Could not save the age."));
		return;
	}
	Reload(imageId);
	m_status.SetWindowText(_T("Age saved."));
}

void CEditDatabaseDlg::OnChangeSpecies()
{
	const long long imageId = SelectedImageId();
	if (imageId == 0)
		return;

	CAssignSpeciesDlg dialog(m_database, this);
	if (dialog.DoModal() != IDOK)
		return;
	if (!m_database.SetImageSpecies(imageId, dialog.GetSpeciesId()))
	{
		m_status.SetWindowText(_T("Could not link the image to that species."));
		return;
	}
	Reload(imageId);
	m_status.SetWindowText(_T("Image linked."));
}
