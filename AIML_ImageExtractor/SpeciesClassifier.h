#pragma once

#include "SpeciesDatabase.h"
#include <opencv2/dnn.hpp>

struct SpeciesPrediction
{
	SpeciesRecord species;
	float confidence = 0.0f;
};

class CSpeciesClassifier
{
public:
	bool Predict(const cv::Mat& image, SpeciesPrediction& prediction);
	const CString& GetLastError() const { return m_lastError; }

private:
	static CString FindModel();
	bool EnsureLoaded();

	cv::dnn::Net m_net;
	std::vector<SpeciesRecord> m_classes;
	bool m_loaded = false;
	CString m_lastError;
};
