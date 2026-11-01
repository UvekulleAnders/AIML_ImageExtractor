#include "pch.h"
#include "framework.h"
#include "SpeciesClassifier.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace
{
	const cv::Size kInputSize(224, 224);
	const float kMean[] = { 0.485f, 0.456f, 0.406f };
	const float kStd[] = { 0.229f, 0.224f, 0.225f };

	CString FromUtf8(const std::string& value)
	{
		const int length = ::MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
		CString result;
		if (length > 0)
		{
			::MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.GetBuffer(length), length);
			result.ReleaseBuffer(length);
		}
		return result;
	}

	bool ReadString(const cv::FileNode& node, const char* key, CString& value)
	{
		std::string text;
		node[key] >> text;
		value = FromUtf8(text);
		return !value.IsEmpty();
	}
}

bool ReadSpeciesClassifierLabels(const CString& path, std::vector<SpeciesRecord>& classes, CString& error)
{
	classes.clear();
	try
	{
		cv::FileStorage labels(CStringA(path).GetString(), cv::FileStorage::READ);
		if (!labels.isOpened())
		{
			error = _T("Could not read the species classifier labels file.");
			return false;
		}

		int formatVersion = 0;
		labels["format_version"] >> formatVersion;
		if (formatVersion != 1)
		{
			error = _T("The species classifier labels file has an unsupported format.");
			return false;
		}

		const cv::FileNode names = labels["class_names"];
		const cv::FileNode species = labels["species"];
		if (names.type() != cv::FileNode::SEQ || names.empty() || species.type() != cv::FileNode::MAP)
		{
			error = _T("The species classifier labels file is missing class metadata.");
			return false;
		}

		for (auto it = names.begin(); it != names.end(); ++it)
		{
			std::string className;
			*it >> className;
			const cv::FileNode metadata = species[className];
			SpeciesRecord record;
			if (className.empty() || metadata.empty() || !ReadString(metadata, "latin", record.latinName) ||
				!ReadString(metadata, "swedish", record.swedishName))
			{
				error = _T("The species classifier labels file has incomplete class metadata.");
				classes.clear();
				return false;
			}
			ReadString(metadata, "english", record.englishName);
			classes.push_back(record);
		}
		error.Empty();
		return true;
	}
	catch (const cv::Exception& e)
	{
		classes.clear();
		error = CString(_T("Could not parse the species classifier labels file: ")) + CString(e.what());
		return false;
	}
}

int FindSpeciesMatch(const std::vector<SpeciesRecord>& species, const SpeciesRecord& prediction)
{
	if (prediction.latinName.IsEmpty())
		return -1;
	for (size_t i = 0; i < species.size(); ++i)
	{
		if (!species[i].latinName.IsEmpty() && species[i].latinName.CompareNoCase(prediction.latinName) == 0)
			return static_cast<int>(i);
	}
	return -1;
}

bool SelectTopSpecies(const std::vector<float>& logits, const std::vector<SpeciesRecord>& classes,
	SpeciesPrediction& prediction)
{
	if (logits.empty() || logits.size() != classes.size())
		return false;

	int best = 0;
	float maximum = logits[0];
	if (!std::isfinite(maximum))
		return false;
	for (size_t i = 1; i < logits.size(); ++i)
	{
		if (!std::isfinite(logits[i]))
			return false;
		if (logits[i] > maximum)
		{
			maximum = logits[i];
			best = static_cast<int>(i);
		}
	}
	double total = 0.0;
	for (const float value : logits)
		total += std::exp(static_cast<double>(value - maximum));
	if (!std::isfinite(total) || total <= 0.0)
		return false;

	prediction.species = classes[best];
	prediction.confidence = static_cast<float>(1.0 / total);
	return true;
}

CString CSpeciesClassifier::FindModel()
{
	TCHAR exePath[MAX_PATH] = {};
	::GetModuleFileName(nullptr, exePath, MAX_PATH);
	CString dir(exePath);
	for (int level = 0; level < 5; ++level)
	{
		const int slash = dir.ReverseFind(_T('\\'));
		if (slash < 0)
			break;
		dir = dir.Left(slash);
		const CString candidate = dir + _T("\\models\\species_model.onnx");
		if (::GetFileAttributes(candidate) != INVALID_FILE_ATTRIBUTES)
			return candidate;
	}
	return CString();
}

bool CSpeciesClassifier::EnsureLoaded()
{
	if (m_loaded)
		return true;

	const CString modelPath = m_modelPath.IsEmpty() ? FindModel() : m_modelPath;
	if (modelPath.IsEmpty())
	{
		m_lastError = _T("Species classifier not found. Put species_model.onnx and species_model.labels.json in a 'models' folder (see README).");
		return false;
	}

	CString labelsPath(modelPath);
	const int dot = labelsPath.ReverseFind(_T('.'));
	if (dot >= 0)
		labelsPath = labelsPath.Left(dot) + _T(".labels.json");
	if (::GetFileAttributes(labelsPath) == INVALID_FILE_ATTRIBUTES)
	{
		m_lastError = _T("Species classifier labels file was not found beside the ONNX model.");
		return false;
	}

	try
	{
		std::vector<SpeciesRecord> parsedClasses;
		CString labelsError;
		if (!ReadSpeciesClassifierLabels(labelsPath, parsedClasses, labelsError))
		{
			m_lastError = labelsError;
			return false;
		}

		m_net = cv::dnn::readNetFromONNX(std::string(CStringA(modelPath).GetString()));
		m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
		m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
		m_classes = std::move(parsedClasses);
		m_loaded = true;
		m_lastError.Empty();
		return true;
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Could not load the species classifier: ")) + CString(e.what());
		return false;
	}
}

bool CSpeciesClassifier::Predict(const cv::Mat& image, SpeciesPrediction& prediction)
{
	if (image.empty() || !EnsureLoaded())
		return false;

	try
	{
		cv::Mat bgr;
		if (image.channels() == 1)
			cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
		else if (image.channels() == 4)
			cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
		else if (image.channels() == 3)
			bgr = image;
		else
		{
			m_lastError = _T("The selected crop has an unsupported number of color channels.");
			return false;
		}

		cv::Mat resized, rgb, normalized;
		cv::resize(bgr, resized, kInputSize, 0.0, 0.0, cv::INTER_LINEAR);
		cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
		rgb.convertTo(normalized, CV_32FC3, 1.0 / 255.0);
		std::vector<cv::Mat> channels;
		cv::split(normalized, channels);
		for (int channel = 0; channel < 3; ++channel)
			channels[channel] = (channels[channel] - kMean[channel]) / kStd[channel];
		cv::merge(channels, normalized);

		m_net.setInput(cv::dnn::blobFromImage(normalized));
		const cv::Mat logits = m_net.forward().reshape(1, 1);
		if (logits.total() != m_classes.size() || logits.depth() != CV_32F)
		{
			m_lastError = _T("The classifier output does not match its labels file.");
			return false;
		}
		const float* values = logits.ptr<float>();
		const std::vector<float> valuesVector(values, values + logits.total());
		if (!SelectTopSpecies(valuesVector, m_classes, prediction))
		{
			m_lastError = _T("The classifier returned invalid prediction scores.");
			return false;
		}
		m_lastError.Empty();
		return true;
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Could not classify the selected crop: ")) + CString(e.what());
		return false;
	}
}
