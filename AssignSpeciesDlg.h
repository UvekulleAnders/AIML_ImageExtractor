// AssignSpeciesDlg.h : asks which species an extracted image belongs to
//

#pragma once

#include "SpeciesDatabase.h"

// The dialog template and its controls are built in code, so no resource entries are needed.
class CAssignSpeciesDlg : public CDialog
{
public:
	explicit CAssignSpeciesDlg(CSpeciesDatabase& database, CWnd* parent = nullptr);

	// Id of the species chosen (or created) when the dialog was closed with OK.
	long long GetSpeciesId() const { return m_speciesId; }

	INT_PTR DoModal() override;

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;

	afx_msg void OnModeChanged();
	afx_msg void OnSuggest();
	DECLARE_MESSAGE_MAP()

private:
	void PlaceControl(CWnd& control, int x, int y, int cx, int cy);
	void UpdateEnabledState();
	bool IsNewSpecies() const;

	CSpeciesDatabase& m_database;
	std::vector<SpeciesRecord> m_species;
	long long m_speciesId = kUnknownSpeciesId;

	CStatic m_intro;
	CButton m_existingRadio;
	CComboBox m_speciesCombo;
	CButton m_newRadio;
	CStatic m_swedishLabel, m_latinLabel, m_englishLabel;
	CEdit m_swedishEdit, m_latinEdit, m_englishEdit;
	CButton m_suggestButton;
	CStatic m_status;
	CButton m_okButton, m_cancelButton;
};
