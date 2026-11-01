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
	// hints are motion candidates: each is re-examined zoomed in. Hints YOLOX does not confirm are
	// returned in unconfirmed. Boxes are widened by the box padding on every side.
	std::vector<cv::Rect> DetectBirds(const cv::Mat& frame, const std::vector<cv::Rect>& hints,
		std::vector<cv::Rect>& unconfirmed, float minScore = 0.15f);
	void SetBoxPadding(int pixels) { m_padding = pixels; }
	int GetBoxPadding() const { return m_padding; }
	const CString& GetLastError() const { return m_lastError; }

private:
	static CString FindModel();
	void RunModel(const cv::Mat& bgr, const cv::Point& offset, float minScore,
		std::vector<cv::Rect>& boxes, std::vector<float>& scores);
	cv::dnn::Net m_net;
	bool m_loaded = false;
	int m_padding = 3; // pixels added around each box to leave a frame around the bird
	CString m_lastError;
};