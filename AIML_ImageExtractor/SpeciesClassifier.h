#pragma once

#include "SpeciesDatabase.h"
#include <opencv2/dnn.hpp>

struct SpeciesPrediction
{
	SpeciesRecord species;
	float confidence = 0.0f;
};

bool ReadSpeciesClassifierLabels(const CString& path, std::vector<SpeciesRecord>& classes, CString& error);
int FindSpeciesMatch(const std::vector<SpeciesRecord>& species, const SpeciesRecord& prediction);
bool SelectTopSpecies(const std::vector<float>& logits, const std::vector<SpeciesRecord>& classes,
	SpeciesPrediction& prediction);

class CSpeciesClassifier
{
public:
	explicit CSpeciesClassifier(const CString& modelPath = CString()) : m_modelPath(modelPath) {}
	bool Predict(const cv::Mat& image, SpeciesPrediction& prediction);
	const CString& GetLastError() const { return m_lastError; }

private:
	static CString FindModel();
	bool EnsureLoaded();

	cv::dnn::Net m_net;
	std::vector<SpeciesRecord> m_classes;
	CString m_modelPath;
	bool m_loaded = false;
	CString m_lastError;
};
