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

	const CString modelPath = FindModel();
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
		cv::FileStorage labels(CStringA(labelsPath).GetString(), cv::FileStorage::READ);
		if (!labels.isOpened())
		{
			m_lastError = _T("Could not read the species classifier labels file.");
			return false;
		}

		int formatVersion = 0;
		labels["format_version"] >> formatVersion;
		if (formatVersion != 1)
		{
			m_lastError = _T("The species classifier labels file has an unsupported format.");
			return false;
		}

		const cv::FileNode classes = labels["class_names"];
		const cv::FileNode species = labels["species"];
		if (classes.type() != cv::FileNode::SEQ || classes.empty() || species.type() != cv::FileNode::MAP)
		{
			m_lastError = _T("The species classifier labels file is missing class metadata.");
			return false;
		}

		std::vector<SpeciesRecord> parsedClasses;
		for (auto it = classes.begin(); it != classes.end(); ++it)
		{
			std::string className;
			*it >> className;
			const cv::FileNode metadata = species[className];
			SpeciesRecord record;
			if (metadata.empty() || !ReadString(metadata, "latin", record.latinName) ||
				!ReadString(metadata, "swedish", record.swedishName))
			{
				m_lastError = _T("The species classifier labels file has incomplete class metadata.");
				return false;
			}
			ReadString(metadata, "english", record.englishName);
			parsedClasses.push_back(record);
		}
		if (parsedClasses.empty())
		{
			m_lastError = _T("The species classifier labels file contains no classes.");
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
		int best = 0;
		float maximum = values[0];
		for (int i = 1; i < logits.cols; ++i)
		{
			if (values[i] > maximum)
			{
				maximum = values[i];
				best = i;
			}
		}
		double total = 0.0;
		for (int i = 0; i < logits.cols; ++i)
			total += std::exp(static_cast<double>(values[i] - maximum));

		prediction.species = m_classes[best];
		prediction.confidence = total > 0.0 ? static_cast<float>(1.0 / total) : 0.0f;
		m_lastError.Empty();
		return true;
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Could not classify the selected crop: ")) + CString(e.what());
		return false;
	}
}
