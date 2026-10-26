#include "pch.h"
#include "framework.h"
#include "MotionCandidates.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <algorithm>

namespace
{
	const int kWorkWidth = 480;
	const int kMinFramesForBackground = 6;
}

void CMotionCandidates::Push(const cv::Mat& frame, double index)
{
	if (frame.empty())
		return;
	if (index != m_last + 1.0)
		m_frames.clear();

	cv::Mat bgr;
	if (frame.channels() == 1)
		cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
	else if (frame.channels() == 4)
		cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
	else
		bgr = frame;

	m_fullSize = bgr.size();
	m_scale = std::min(1.0, kWorkWidth / static_cast<double>(bgr.cols));
	cv::Mat small, lab;
	cv::resize(bgr, small, cv::Size(), m_scale, m_scale, cv::INTER_AREA);
	cv::GaussianBlur(small, small, cv::Size(5, 5), 0);
	cv::cvtColor(small, lab, cv::COLOR_BGR2Lab); // colour change, less sensitive to brightness shifts

	m_frames.push_back(lab);
	while (static_cast<int>(m_frames.size()) > kMaxFrames + 1)
		m_frames.pop_front();
	m_last = index;
}

std::vector<cv::Rect> CMotionCandidates::Find()
{
	std::vector<cv::Rect> result;
	if (static_cast<int>(m_frames.size()) < kMinFramesForBackground + 1)
		return result;

	try
	{
		cv::Ptr<cv::BackgroundSubtractorMOG2> model = cv::createBackgroundSubtractorMOG2(kMaxFrames, 25.0, false);
		cv::Mat mask;
		for (size_t i = 0; i + 1 < m_frames.size(); ++i)
			model->apply(m_frames[i], mask);
		model->apply(m_frames.back(), mask, 0.0); // learning rate 0: the newest frame is only compared

		cv::threshold(mask, mask, 200, 255, cv::THRESH_BINARY);
		cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9)));

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		const double imageArea = static_cast<double>(mask.cols) * mask.rows;
		std::vector<std::pair<double, cv::Rect>> found;
		for (const auto& contour : contours)
		{
			const double area = cv::contourArea(contour);
			if (area < 12.0 || area > 0.2 * imageArea)
				continue;
			const cv::Rect box = cv::boundingRect(contour);
			const double aspect = box.width / static_cast<double>(box.height);
			if (aspect < 0.25 || aspect > 4.0)
				continue;
			std::vector<cv::Point> hull;
			cv::convexHull(contour, hull);
			const double hullArea = cv::contourArea(hull);
			if (hullArea <= 0.0 || area / hullArea < 0.35) // thin or ragged shapes are usually branches or noise
				continue;
			found.push_back(std::make_pair(area, box));
		}
		std::sort(found.begin(), found.end(),
			[](const auto& a, const auto& b) { return a.first > b.first; });
		for (size_t i = 0; i < found.size() && i < 8; ++i)
		{
			const cv::Rect& b = found[i].second;
			result.push_back(cv::Rect(cvRound(b.x / m_scale), cvRound(b.y / m_scale),
				cvRound(b.width / m_scale), cvRound(b.height / m_scale)) & cv::Rect(cv::Point(0, 0), m_fullSize));
		}
	}
	catch (const cv::Exception&)
	{
		result.clear();
	}
	return result;
}