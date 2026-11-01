#include "pch.h"
#include <gtest/gtest.h>
#include "SpeciesClassifier.h"
#include "TestUtil.h"
#include <fstream>
#include <limits>

namespace
{
	SpeciesRecord Species(const wchar_t* swedish, const wchar_t* latin)
	{
		SpeciesRecord species;
		species.swedishName = swedish;
		species.latinName = latin;
		return species;
	}

	void WriteLabels(const CString& path, const char* json)
	{
		std::ofstream file(std::wstring(path.GetString()), std::ios::binary);
		file << json;
	}
}

TEST(SpeciesClassifier, ReadsTrainerLabelsAndSpeciesMetadata)
{
	TempDir dir;
	const CString path = dir.File(L"model.labels.json");
	WriteLabels(path,
		"{\"format_version\":1,\"class_names\":[\"Talgoxe__Parus_major\"],"
		"\"species\":{\"Talgoxe__Parus_major\":{\"species_id\":\"2\","
		"\"swedish\":\"Talgoxe\",\"latin\":\"Parus major\",\"english\":\"Great tit\"}}}");

	std::vector<SpeciesRecord> classes;
	CString error;
	ASSERT_TRUE(ReadSpeciesClassifierLabels(path, classes, error)) << CStringA(error).GetString();
	ASSERT_EQ(1u, classes.size());
	EXPECT_EQ(CString(L"Talgoxe"), classes[0].swedishName);
	EXPECT_EQ(CString(L"Parus major"), classes[0].latinName);
	EXPECT_EQ(CString(L"Great tit"), classes[0].englishName);
	EXPECT_TRUE(error.IsEmpty());
}

TEST(SpeciesClassifier, RejectsIncompleteLabels)
{
	TempDir dir;
	const CString path = dir.File(L"invalid.labels.json");
	WriteLabels(path, "{\"format_version\":1,\"class_names\":[\"Missing\"],\"species\":{}}");

	std::vector<SpeciesRecord> classes;
	CString error;
	EXPECT_FALSE(ReadSpeciesClassifierLabels(path, classes, error));
	EXPECT_TRUE(classes.empty());
	EXPECT_FALSE(error.IsEmpty());
}

TEST(SpeciesClassifier, MatchesLocalSpeciesByLatinNameIgnoringCase)
{
	const std::vector<SpeciesRecord> local = {
		Species(L"Unknown", L""),
		Species(L"Talgoxe", L"Parus major")
	};
	EXPECT_EQ(1, FindSpeciesMatch(local, Species(L"Great tit", L"PARUS MAJOR")));
	EXPECT_EQ(-1, FindSpeciesMatch(local, Species(L"Other bird", L"Turdus merula")));
	EXPECT_EQ(-1, FindSpeciesMatch(local, Species(L"Talgoxe", L"")));
}

TEST(SpeciesClassifier, SelectsTopLogitAndCalculatesConfidence)
{
	const std::vector<SpeciesRecord> classes = {
		Species(L"Talgoxe", L"Parus major"),
		Species(L"Koltrast", L"Turdus merula")
	};
	SpeciesPrediction prediction;
	ASSERT_TRUE(SelectTopSpecies({ 1.0f, 2.0f }, classes, prediction));
	EXPECT_EQ(CString(L"Turdus merula"), prediction.species.latinName);
	EXPECT_NEAR(0.731f, prediction.confidence, 0.001f);
}

TEST(SpeciesClassifier, RejectsInvalidPredictionScores)
{
	SpeciesPrediction prediction;
	const std::vector<SpeciesRecord> classes = { Species(L"Talgoxe", L"Parus major") };
	EXPECT_FALSE(SelectTopSpecies({}, classes, prediction));
	EXPECT_FALSE(SelectTopSpecies({ 1.0f, 2.0f }, classes, prediction));
	EXPECT_FALSE(SelectTopSpecies({ std::numeric_limits<float>::quiet_NaN() }, classes, prediction));
}

TEST(SpeciesClassifier, MissingModelFallsBackWithAnError)
{
	TempDir dir;
	CSpeciesClassifier classifier(dir.File(L"missing.onnx"));
	SpeciesPrediction prediction;
	const cv::Mat crop(16, 16, CV_8UC3, cv::Scalar(0, 0, 0));
	EXPECT_FALSE(classifier.Predict(crop, prediction));
	EXPECT_FALSE(classifier.GetLastError().IsEmpty());
}
