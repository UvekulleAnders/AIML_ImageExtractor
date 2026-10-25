#include "pch.h"
#include "framework.h"
#include "ObjectDetector.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace
{
	const int kInputSize = 640;
	const int kBirdClass = 14; // COCO class index of 'bird'
	const int kClassCount = 80;
}

CString CObjectDetector::FindModel()
{
	// The model is searched for in 'models' next to the exe and in the parent folders.
	TCHAR exePath[MAX_PATH] = {};
	::GetModuleFileName(nullptr, exePath, MAX_PATH);
	CString dir(exePath);
	for (int level = 0; level < 5; ++level)
	{
		const int slash = dir.ReverseFind(_T('\\'));
		if (slash < 0)
			break;
		dir = dir.Left(slash);
		const CString candidate = dir + _T("\\models\\object_detection_yolox_2022nov.onnx");
		if (::GetFileAttributes(candidate) != INVALID_FILE_ATTRIBUTES)
			return candidate;
	}
	return CString();
}

bool CObjectDetector::EnsureLoaded()
{
	if (m_loaded)
		return true;
	const CString path = FindModel();
	if (path.IsEmpty())
	{
		m_lastError = _T("Detection model not found. Put object_detection_yolox_2022nov.onnx in a 'models' folder (see README).");
		return false;
	}
	try
	{
		m_net = cv::dnn::readNetFromONNX(std::string(CStringA(path).GetString()));
		m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
		m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Could not load the detection model: ")) + CString(e.what());
		return false;
	}
	m_loaded = true;
	return true;
}

void CObjectDetector::RunModel(const cv::Mat& bgr, const cv::Point& offset, float minScore,
	std::vector<cv::Rect>& boxes, std::vector<float>& scores)
{
	// Letterbox: keep aspect ratio, pad with 114 at the bottom/right.
	const float ratio = std::min(kInputSize / static_cast<float>(bgr.cols), kInputSize / static_cast<float>(bgr.rows));
	cv::Mat resized;
	cv::resize(bgr, resized, cv::Size(), ratio, ratio, cv::INTER_LINEAR);
	cv::Mat padded(kInputSize, kInputSize, CV_8UC3, cv::Scalar(114, 114, 114));
	resized.copyTo(padded(cv::Rect(0, 0, resized.cols, resized.rows)));

	cv::Mat blob = cv::dnn::blobFromImage(padded, 1.0, cv::Size(kInputSize, kInputSize), cv::Scalar(), false, false);
	m_net.setInput(blob);
	cv::Mat out = m_net.forward();
	out = out.reshape(1, static_cast<int>(out.total() / (kClassCount + 5))); // [N, 85]

	const int strides[3] = { 8, 16, 32 };
	int row = 0;
	for (int s = 0; s < 3; ++s)
	{
		const int grid = kInputSize / strides[s];
		for (int gy = 0; gy < grid; ++gy)
		{
			for (int gx = 0; gx < grid; ++gx, ++row)
			{
				if (row >= out.rows)
					return;
				const float* p = out.ptr<float>(row);
				const float score = p[4] * p[5 + kBirdClass];
				if (score < minScore)
					continue;
				const float cx = (p[0] + gx) * strides[s];
				const float cy = (p[1] + gy) * strides[s];
				const float w = std::exp(p[2]) * strides[s];
				const float h = std::exp(p[3]) * strides[s];
				const cv::Rect box(
					cvRound((cx - w / 2) / ratio) + offset.x, cvRound((cy - h / 2) / ratio) + offset.y,
					cvRound(w / ratio), cvRound(h / ratio));
				boxes.push_back(box);
				scores.push_back(score);
			}
		}
	}
}

std::vector<cv::Rect> CObjectDetector::DetectBirds(const cv::Mat& frame, float minScore)
{
	std::vector<cv::Rect> result;
	if (frame.empty() || !EnsureLoaded())
		return result;

	try
	{
		cv::Mat bgr;
		if (frame.channels() == 1)
			cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
		else if (frame.channels() == 4)
			cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
		else
			bgr = frame;

		std::vector<cv::Rect> boxes;
		std::vector<float> scores;
		RunModel(bgr, cv::Point(0, 0), minScore, boxes, scores);

		// Birds at a feeder are small: also scan 2x2 overlapping tiles so they are seen at a larger scale.
		const int tileW = bgr.cols * 3 / 5, tileH = bgr.rows * 3 / 5;
		for (int ty = 0; ty < 2; ++ty)
			for (int tx = 0; tx < 2; ++tx)
			{
				const cv::Rect tile(tx * (bgr.cols - tileW), ty * (bgr.rows - tileH), tileW, tileH);
				RunModel(bgr(tile), tile.tl(), minScore, boxes, scores);
			}

		const cv::Rect bounds(0, 0, bgr.cols, bgr.rows);
		for (cv::Rect& box : boxes)
			box &= bounds;
		std::vector<int> keep;
		cv::dnn::NMSBoxes(boxes, scores, minScore, 0.45f, keep);
		for (int index : keep)
			if (boxes[index].width >= 8 && boxes[index].height >= 8)
				result.push_back(boxes[index]);
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Detection failed: ")) + CString(e.what());
		result.clear();
	}
	return result;
}