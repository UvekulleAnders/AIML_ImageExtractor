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

// A motion blob often covers only the moving part of the bird: segment the colour region around it with GrabCut.
static cv::Rect GrowBlob(const cv::Mat& bgr, const cv::Rect& hint)
{
	try
	{
		const int margin = std::max(40, std::max(hint.width, hint.height));
		const cv::Rect bounds(0, 0, bgr.cols, bgr.rows);
		const cv::Rect win = cv::Rect(hint.x - margin, hint.y - margin, hint.width + 2 * margin, hint.height + 2 * margin) & bounds;
		if (win.width < 24 || win.height < 24)
			return hint;
		cv::Mat mask(win.size(), CV_8UC1, cv::Scalar(cv::GC_PR_BGD));
		const cv::Rect local = (hint & win) - win.tl();
		mask(local).setTo(cv::GC_PR_FGD);
		const cv::Rect core(local.x + local.width / 4, local.y + local.height / 4, std::max(1, local.width / 2), std::max(1, local.height / 2));
		mask(core).setTo(cv::GC_FGD);
		const int edge = 4;
		mask.rowRange(0, edge).setTo(cv::GC_BGD);
		mask.rowRange(win.height - edge, win.height).setTo(cv::GC_BGD);
		mask.colRange(0, edge).setTo(cv::GC_BGD);
		mask.colRange(win.width - edge, win.width).setTo(cv::GC_BGD);
		cv::Mat bg, fg;
		cv::grabCut(bgr(win), mask, cv::Rect(), bg, fg, 3, cv::GC_INIT_WITH_MASK);
		cv::Mat fgMask = (mask == cv::GC_FGD) | (mask == cv::GC_PR_FGD);
		cv::Mat labels, stats, centroids;
		const int n = cv::connectedComponentsWithStats(fgMask, labels, stats, centroids);
		const int id = labels.at<int>(core.y + core.height / 2, core.x + core.width / 2);
		if (id <= 0 || n < 2)
			return hint;
		const cv::Rect grown(stats.at<int>(id, cv::CC_STAT_LEFT), stats.at<int>(id, cv::CC_STAT_TOP),
			stats.at<int>(id, cv::CC_STAT_WIDTH), stats.at<int>(id, cv::CC_STAT_HEIGHT));
		if (grown.area() > 12 * hint.area() + 2000)
			return hint;
		return (grown + win.tl()) | hint;
	}
	catch (const cv::Exception&)
	{
		return hint;
	}
}

std::vector<cv::Rect> CObjectDetector::DetectBirds(const cv::Mat& frame, const std::vector<cv::Rect>& hints,
	std::vector<cv::Rect>& unconfirmed, float minScore)
{
	std::vector<cv::Rect> result;
	unconfirmed.clear();
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
		const cv::Rect bounds(0, 0, bgr.cols, bgr.rows);

		const float strongScore = hints.empty() ? minScore : std::min(minScore, 0.10f);
		const float weakScore = 0.03f; // blurred or distant birds score low; keep them as unconfirmed
		std::vector<cv::Rect> boxes;
		std::vector<float> scores;
		RunModel(bgr, cv::Point(0, 0), weakScore, boxes, scores);

		if (hints.empty())
		{
			// No motion info (e.g. a still bird): scan 2x2 overlapping tiles so small birds appear larger.
			const int tileW = bgr.cols * 3 / 5, tileH = bgr.rows * 3 / 5;
			for (int ty = 0; ty < 2; ++ty)
				for (int tx = 0; tx < 2; ++tx)
				{
					const cv::Rect tile(tx * (bgr.cols - tileW), ty * (bgr.rows - tileH), tileW, tileH);
					RunModel(bgr(tile), tile.tl(), weakScore, boxes, scores);
				}
		}
		else
		{
			// Zoom in on each motion candidate: a square crop with context, upscaled by the letterbox.
			for (const cv::Rect& hint : hints)
			{
				int side = std::max(96, std::max(hint.width, hint.height) * 3 / 2);
				side = std::min(side, std::min(bgr.cols, bgr.rows));
				const cv::Point centre(hint.x + hint.width / 2, hint.y + hint.height / 2);
				cv::Rect crop(centre.x - side / 2, centre.y - side / 2, side, side);
				crop.x = std::max(0, std::min(crop.x, bgr.cols - side));
				crop.y = std::max(0, std::min(crop.y, bgr.rows - side));
				RunModel(bgr(crop), crop.tl(), weakScore, boxes, scores);
			}
		}

		for (cv::Rect& box : boxes)
			box &= bounds;
		std::vector<int> keep;
		cv::dnn::NMSBoxes(boxes, scores, weakScore, 0.45f, keep);
		std::vector<cv::Rect> weak;
		for (int index : keep)
			if (boxes[index].width >= 8 && boxes[index].height >= 8)
			{
				if (scores[index] >= strongScore)
					result.push_back(boxes[index]);
				else if (weak.size() < 4)
					weak.push_back(boxes[index]);
			}
		unconfirmed = weak;

		for (const cv::Rect& hint : hints)
		{
			bool confirmed = false;
			for (const cv::Rect& bird : result)
				if (!(bird & hint).empty())
					confirmed = true;
			if (!confirmed)
			{
				bool dup = false;
				for (const cv::Rect& u : unconfirmed)
					if (!(u & hint).empty())
						dup = true;
				if (!dup)
				{
					unconfirmed.push_back(GrowBlob(bgr, hint));
				}
			}
		}

		auto widen = [&](cv::Rect& box)
		{
			box = cv::Rect(box.x - m_padding, box.y - m_padding, box.width + 2 * m_padding, box.height + 2 * m_padding) & bounds;
		};
		for (cv::Rect& box : result)
			widen(box);
		for (cv::Rect& box : unconfirmed)
			widen(box);
	}
	catch (const cv::Exception& e)
	{
		m_lastError = CString(_T("Detection failed: ")) + CString(e.what());
		result.clear();
		unconfirmed.clear();
	}
	return result;
}