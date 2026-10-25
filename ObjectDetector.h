// ObjectDetector.h : bird detection in a video frame using a YOLOX ONNX model (OpenCV dnn)
//

#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

class CObjectDetector
{
public:
	// Loads the model on first use. Returns false and sets GetLastError() if it is not found.
	bool EnsureLoaded();
	// Returns bounding boxes (in frame pixels) of birds found in the frame.
	std::vector<cv::Rect> DetectBirds(const cv::Mat& frame, float minScore = 0.15f);
	const CString& GetLastError() const { return m_lastError; }

private:
	static CString FindModel();
	void RunModel(const cv::Mat& bgr, const cv::Point& offset, float minScore,
		std::vector<cv::Rect>& boxes, std::vector<float>& scores);
	cv::dnn::Net m_net;
	bool m_loaded = false;
	CString m_lastError;
};