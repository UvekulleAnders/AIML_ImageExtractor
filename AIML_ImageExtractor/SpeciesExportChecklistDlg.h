#pragma once

#include <afxdialogex.h>
#include "SpeciesDatabase.h"
#include "SpeciesExportChecklistDlgResource.h"
#include <vector>

class CSpeciesExportChecklistDlg : public CDialogEx
{
public:
	CSpeciesExportChecklistDlg(const std::vector<SpeciesEntryCount>& species, CWnd* pParent = nullptr);
	const std::vector<long long>& GetSelectedSpeciesIds() const { return m_selectedSpeciesIds; }

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelectAll();
	afx_msg void OnSelectNone();
	DECLARE_MESSAGE_MAP()

private:
	const std::vector<SpeciesEntryCount>& m_species;
	CCheckListBox m_speciesList;
	std::vector<long long> m_selectedSpeciesIds;
};
