#include "pch.h"
#include <gtest/gtest.h>
#include <opencv2/imgproc.hpp>
#include "MotionCandidates.h"

namespace
{
	// A flat grey scene, optionally with a bright square.
	cv::Mat Scene(const cv::Rect* square = nullptr)
	{
		cv::Mat frame(360, 640, CV_8UC3, cv::Scalar(90, 100, 110));
		if (square != nullptr)
			cv::rectangle(frame, *square, cv::Scalar(30, 200, 220), cv::FILLED);
		return frame;
	}
}

TEST(MotionCandidates, StartsEmpty)
{
	CMotionCandidates motion;
	EXPECT_EQ(0, motion.Count());
	EXPECT_TRUE(motion.Find().empty());
}

TEST(MotionCandidates, IgnoresEmptyFrames)
{
	CMotionCandidates motion;
	motion.Push(cv::Mat(), 0);
	EXPECT_EQ(0, motion.Count());
}

TEST(MotionCandidates, NeedsHistoryBeforeFinding)
{
	CMotionCandidates motion;
	for (int i = 0; i < 3; ++i)
		motion.Push(Scene(), i);
	EXPECT_EQ(3, motion.Count());
	EXPECT_TRUE(motion.Find().empty());
}

TEST(MotionCandidates, NonSequentialIndexRestartsHistory)
{
	CMotionCandidates motion;
	for (int i = 0; i < 5; ++i)
		motion.Push(Scene(), i);
	motion.Push(Scene(), 50);
	EXPECT_EQ(1, motion.Count());
	EXPECT_DOUBLE_EQ(50.0, motion.LastIndex());
}

TEST(MotionCandidates, HistoryIsLimited)
{
	CMotionCandidates motion;
	for (int i = 0; i < 100; ++i)
		motion.Push(Scene(), i);
	EXPECT_EQ(CMotionCandidates::kMaxFrames + 1, motion.Count());
}

TEST(MotionCandidates, ClearResetsState)
{
	CMotionCandidates motion;
	for (int i = 0; i < 5; ++i)
		motion.Push(Scene(), i);
	motion.Clear();
	EXPECT_EQ(0, motion.Count());
}

TEST(MotionCandidates, StaticSceneHasNoCandidates)
{
	CMotionCandidates motion;
	for (int i = 0; i < 12; ++i)
		motion.Push(Scene(), i);
	EXPECT_TRUE(motion.Find().empty());
}

TEST(MotionCandidates, FindsNewObjectInFullFrameCoordinates)
{
	CMotionCandidates motion;
	for (int i = 0; i < 12; ++i)
		motion.Push(Scene(), i);
	const cv::Rect bird(400, 150, 60, 50);
	motion.Push(Scene(&bird), 12);

	const std::vector<cv::Rect> found = motion.Find();
	ASSERT_FALSE(found.empty());
	const cv::Rect overlap = found.front() & bird;
	EXPECT_GT(overlap.area(), bird.area() / 2);
	EXPECT_LE(found.size(), 8u);
}
