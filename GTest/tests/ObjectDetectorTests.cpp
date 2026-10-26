#include "pch.h"
#include <gtest/gtest.h>
#include "ObjectDetector.h"

TEST(ObjectDetector, DefaultPaddingIsThreePixels)
{
	CObjectDetector detector;
	EXPECT_EQ(3, detector.GetBoxPadding());
}

TEST(ObjectDetector, PaddingIsConfigurable)
{
	CObjectDetector detector;
	detector.SetBoxPadding(7);
	EXPECT_EQ(7, detector.GetBoxPadding());
}

TEST(ObjectDetector, EmptyFrameGivesNoDetections)
{
	CObjectDetector detector;
	std::vector<cv::Rect> unconfirmed;
	EXPECT_TRUE(detector.DetectBirds(cv::Mat(), {}, unconfirmed).empty());
	EXPECT_TRUE(unconfirmed.empty());
}

TEST(ObjectDetector, BlankFrameHasNoBirds)
{
	CObjectDetector detector;
	if (!detector.EnsureLoaded())
		GTEST_SKIP() << "YOLOX model not installed: " << CStringA(detector.GetLastError()).GetString();
	const cv::Mat blank(720, 1280, CV_8UC3, cv::Scalar(128, 128, 128));
	std::vector<cv::Rect> unconfirmed;
	EXPECT_TRUE(detector.DetectBirds(blank, {}, unconfirmed).empty());
}
