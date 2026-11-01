// EditDatabaseDlg.h : edits stored images and species (complete names, relink an image to another species)
//

#pragma once

#include "SpeciesDatabase.h"

// The dialog template and its controls are built in code, so no resource entries are needed.
class CEditDatabaseDlg : public CDialog
{
public:
	explicit CEditDatabaseDlg(CSpeciesDatabase& database, CWnd* parent = nullptr);

	INT_PTR DoModal() override;

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnImageSelected(NMHDR* header, LRESULT* result);
	afx_msg void OnSaveSpecies();
	afx_msg void OnChangeSpecies();
	afx_msg void OnAgeChanged();
	DECLARE_MESSAGE_MAP()

private:
	void PlaceControl(CWnd& control, int x, int y, int cx, int cy);
	void Reload(long long selectImageId);
	long long SelectedImageId() const;
	const SpeciesRecord* FindSpecies(long long id) const;
	void UpdateSpeciesFields();

	CSpeciesDatabase& m_database;
	std::vector<SpeciesRecord> m_species;
	std::vector<ImageRecord> m_images;

	CListCtrl m_list;
	CStatic m_speciesGroup, m_swedishLabel, m_latinLabel, m_englishLabel, m_ageLabel, m_status;
	CComboBox m_ageCombo;
	CEdit m_swedishEdit, m_latinEdit, m_englishEdit;
	CButton m_saveButton, m_changeButton, m_closeButton;
};
