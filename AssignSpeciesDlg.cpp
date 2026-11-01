// AssignSpeciesDlg.cpp : implementation of the CAssignSpeciesDlg class
//

#include "pch.h"
#include "AssignSpeciesDlg.h"
#include "WikipediaLookup.h"

namespace
{
	constexpr UINT IDC_EXISTING_RADIO = 2001;
	constexpr UINT IDC_SPECIES_COMBO = 2002;
	constexpr UINT IDC_NEW_RADIO = 2003;
	constexpr UINT IDC_SWEDISH_EDIT = 2004;
	constexpr UINT IDC_LATIN_EDIT = 2005;
	constexpr UINT IDC_ENGLISH_EDIT = 2006;
	constexpr UINT IDC_SUGGEST = 2007;
	constexpr UINT IDC_STATUS = 2008;

	constexpr short kDialogWidth = 280;
	constexpr short kDialogHeight = 172;

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

	CString Describe(const SpeciesRecord& species)
	{
		CString text = species.DisplayName();
		if (!species.latinName.IsEmpty() && species.latinName != text)
			text += _T(" (") + species.latinName + _T(")");
		if (!species.englishName.IsEmpty() && species.englishName != species.DisplayName())
			text += _T(" - ") + species.englishName;
		return text;
	}
}

BEGIN_MESSAGE_MAP(CAssignSpeciesDlg, CDialog)
	ON_BN_CLICKED(IDC_EXISTING_RADIO, &CAssignSpeciesDlg::OnModeChanged)
	ON_BN_CLICKED(IDC_NEW_RADIO, &CAssignSpeciesDlg::OnModeChanged)
	ON_BN_CLICKED(IDC_SUGGEST, &CAssignSpeciesDlg::OnSuggest)
END_MESSAGE_MAP()

CAssignSpeciesDlg::CAssignSpeciesDlg(CSpeciesDatabase& database, CWnd* parent)
	: CDialog(), m_database(database)
{
	m_pParentWnd = parent;
}

INT_PTR CAssignSpeciesDlg::DoModal()
{
	// In-memory DLGTEMPLATE: caption, size and font only; controls are created in OnInitDialog.
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
	AppendString(tmpl, L"Assign species");
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

void CAssignSpeciesDlg::PlaceControl(CWnd& control, int x, int y, int cx, int cy)
{
	CRect rect(x, y, x + cx, y + cy);
	MapDialogRect(&rect);
	control.MoveWindow(rect);
	control.SetFont(GetFont());
}

BOOL CAssignSpeciesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	const DWORD child = WS_CHILD | WS_VISIBLE;
	const DWORD tab = WS_TABSTOP;
	const CRect none(0, 0, 10, 10);

	m_intro.Create(_T("Associate the selected image with:"), child, none, this);
	m_existingRadio.Create(_T("An existing species (default: Unknown)"),
		child | tab | WS_GROUP | BS_AUTORADIOBUTTON, none, this, IDC_EXISTING_RADIO);
	m_speciesCombo.Create(child | tab | WS_VSCROLL | CBS_DROPDOWNLIST, none, this, IDC_SPECIES_COMBO);
	m_newRadio.Create(_T("A new species"), child | BS_AUTORADIOBUTTON, none, this, IDC_NEW_RADIO);
	m_swedishLabel.Create(_T("Swedish name:"), child, none, this);
	m_swedishEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_SWEDISH_EDIT);
	m_latinLabel.Create(_T("Latin name:"), child, none, this);
	m_latinEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_LATIN_EDIT);
	m_englishLabel.Create(_T("English name:"), child, none, this);
	m_englishEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), child | tab | ES_AUTOHSCROLL, none, this, IDC_ENGLISH_EDIT);
	m_suggestButton.Create(_T("Suggest names from Wikipedia"), child | tab | BS_PUSHBUTTON, none, this, IDC_SUGGEST);
	m_status.Create(_T(""), child | SS_LEFTNOWORDWRAP, none, this, IDC_STATUS);
	m_okButton.Create(_T("OK"), child | tab | BS_DEFPUSHBUTTON, none, this, IDOK);
	m_cancelButton.Create(_T("Cancel"), child | tab | BS_PUSHBUTTON, none, this, IDCANCEL);

	PlaceControl(m_intro, 7, 7, 266, 10);
	PlaceControl(m_existingRadio, 7, 20, 200, 10);
	PlaceControl(m_speciesCombo, 14, 32, 259, 120);
	PlaceControl(m_newRadio, 7, 50, 120, 10);
	PlaceControl(m_swedishLabel, 14, 66, 60, 10);
	PlaceControl(m_swedishEdit, 80, 64, 193, 13);
	PlaceControl(m_latinLabel, 14, 82, 60, 10);
	PlaceControl(m_latinEdit, 80, 80, 193, 13);
	PlaceControl(m_englishLabel, 14, 98, 60, 10);
	PlaceControl(m_englishEdit, 80, 96, 193, 13);
	PlaceControl(m_suggestButton, 80, 114, 120, 14);
	PlaceControl(m_status, 14, 133, 259, 10);
	PlaceControl(m_okButton, 165, 151, 50, 14);
	PlaceControl(m_cancelButton, 223, 151, 50, 14);

	m_species = m_database.GetSpecies();
	int selected = 0;
	for (size_t i = 0; i < m_species.size(); ++i)
	{
		const int index = m_speciesCombo.AddString(Describe(m_species[i]));
		m_speciesCombo.SetItemData(index, static_cast<DWORD_PTR>(i));
		if (m_species[i].id == kUnknownSpeciesId)
			selected = index;
	}
	m_speciesCombo.SetCurSel(selected);
	m_existingRadio.SetCheck(BST_CHECKED);
	UpdateEnabledState();

	m_speciesCombo.SetFocus();
	return FALSE;
}

bool CAssignSpeciesDlg::IsNewSpecies() const
{
	return m_newRadio.GetCheck() == BST_CHECKED;
}

void CAssignSpeciesDlg::UpdateEnabledState()
{
	const BOOL isNew = IsNewSpecies();
	m_speciesCombo.EnableWindow(!isNew);
	m_swedishEdit.EnableWindow(isNew);
	m_latinEdit.EnableWindow(isNew);
	m_englishEdit.EnableWindow(isNew);
	m_suggestButton.EnableWindow(isNew);
	if (isNew)
		m_swedishEdit.SetFocus();
	else
		m_speciesCombo.SetFocus();
}

void CAssignSpeciesDlg::OnModeChanged()
{
	UpdateEnabledState();
}

void CAssignSpeciesDlg::OnSuggest()
{
	CString swedish, latin, english, error;
	m_swedishEdit.GetWindowText(swedish);

	CWaitCursor wait;
	m_status.SetWindowText(_T("Looking up on Wikipedia..."));
	m_status.UpdateWindow();

	if (!WikipediaLookup::FindLatinName(swedish, latin, error))
	{
		m_status.SetWindowText(error);
		return;
	}
	m_latinEdit.SetWindowText(latin);

	if (!WikipediaLookup::FindEnglishName(latin, english, error))
	{
		m_status.SetWindowText(_T("Latin name found. ") + error);
		return;
	}
	m_englishEdit.SetWindowText(english);
	m_status.SetWindowText(_T("Suggested from Wikipedia. Edit the names if needed."));
}

void CAssignSpeciesDlg::OnOK()
{
	if (!IsNewSpecies())
	{
		const int index = m_speciesCombo.GetCurSel();
		if (index == CB_ERR)
			return;
		m_speciesId = m_species[m_speciesCombo.GetItemData(index)].id;
		CDialog::OnOK();
		return;
	}

	SpeciesRecord species;
	m_swedishEdit.GetWindowText(species.swedishName);
	m_latinEdit.GetWindowText(species.latinName);
	m_englishEdit.GetWindowText(species.englishName);
	species.swedishName.Trim();
	species.latinName.Trim();
	species.englishName.Trim();
	if (species.latinName.IsEmpty())
	{
		m_status.SetWindowText(_T("Enter the Latin name of the new species (the Swedish and English names are optional)."));
		m_latinEdit.SetFocus();
		return;
	}

	long long newId = 0;
	if (!m_database.AddSpecies(species, &newId))
	{
		m_status.SetWindowText(_T("Could not add the species (does the Latin name already exist?)."));
		return;
	}
	m_speciesId = newId;
	CDialog::OnOK();
}
