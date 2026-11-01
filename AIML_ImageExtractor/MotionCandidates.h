// MotionCandidates.h : finds small moving/changed regions (bird candidates) using colour change against a background model
//

#pragma once

#include <deque>
#include <vector>
#include <opencv2/core.hpp>

class CMotionCandidates
{
public:
	// Adds a frame to the history. A frame that does not follow the previous index restarts the history.
	void Push(const cv::Mat& frame, double index);
	void Clear() { m_frames.clear(); m_last = -100.0; }
	int Count() const { return static_cast<int>(m_frames.size()); }
	double LastIndex() const { return m_last; }
	// Compares the newest frame with a background learned from the earlier ones.
	// Returns candidate rectangles in full-frame pixels.
	std::vector<cv::Rect> Find();

	static const int kMaxFrames = 30;

private:
	std::deque<cv::Mat> m_frames; // small, blurred Lab frames
	double m_last = -100.0;
	double m_scale = 1.0;
	cv::Size m_fullSize;
};