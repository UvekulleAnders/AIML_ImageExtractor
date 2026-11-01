#include "pch.h"
#include "framework.h"
#include "SpeciesExportChecklistDlg.h"
#include "SpeciesExportChecklistDlgResource.h"

BEGIN_MESSAGE_MAP(CSpeciesExportChecklistDlg, CDialogEx)
	ON_BN_CLICKED(IDC_EXPORT_SPECIES_SELECT_ALL, &CSpeciesExportChecklistDlg::OnSelectAll)
	ON_BN_CLICKED(IDC_EXPORT_SPECIES_SELECT_NONE, &CSpeciesExportChecklistDlg::OnSelectNone)
END_MESSAGE_MAP()

CSpeciesExportChecklistDlg::CSpeciesExportChecklistDlg(const std::vector<SpeciesEntryCount>& species, CWnd* pParent)
	: CDialogEx(IDD_EXPORT_SPECIES_DIALOG, pParent), m_species(species)
{
}

BOOL CSpeciesExportChecklistDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	if (!m_speciesList.SubclassDlgItem(IDC_EXPORT_SPECIES_LIST, this))
		return FALSE;

	for (const SpeciesEntryCount& entry : m_species)
	{
		CString label;
		label.Format(_T("%s / %s (%lld entries)"), static_cast<LPCTSTR>(entry.species.swedishName),
			static_cast<LPCTSTR>(entry.species.latinName), entry.entryCount);
		const int index = m_speciesList.AddString(label);
		if (index == LB_ERR || index == LB_ERRSPACE)
			continue;
		m_speciesList.SetItemData(index, static_cast<DWORD_PTR>(entry.species.id));
		m_speciesList.SetCheck(index, TRUE);
	}
	return TRUE;
}

void CSpeciesExportChecklistDlg::OnSelectAll()
{
	for (int i = 0; i < m_speciesList.GetCount(); ++i)
		m_speciesList.SetCheck(i, TRUE);
}

void CSpeciesExportChecklistDlg::OnSelectNone()
{
	for (int i = 0; i < m_speciesList.GetCount(); ++i)
		m_speciesList.SetCheck(i, FALSE);
}

void CSpeciesExportChecklistDlg::OnOK()
{
	m_selectedSpeciesIds.clear();
	for (int i = 0; i < m_speciesList.GetCount(); ++i)
	{
		if (m_speciesList.GetCheck(i) != 0)
			m_selectedSpeciesIds.push_back(static_cast<long long>(m_speciesList.GetItemData(i)));
	}
	if (m_selectedSpeciesIds.empty())
	{
		AfxMessageBox(_T("Select at least one species to export."), MB_ICONINFORMATION);
		return;
	}
	CDialogEx::OnOK();
}
