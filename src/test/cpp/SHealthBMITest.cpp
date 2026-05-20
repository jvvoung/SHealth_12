#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "BmiCalculator.h"
#include "DataImputer.h"
#include "SHealth.h"

namespace {
std::string writeTempCsv(const std::string& content) {
    const std::string path = "test_shealth_temp.csv";
    std::ofstream file(path);
    file << content;
    return path;
}
}  // namespace

TEST(SHealthBmiTest, ComputeBmi) {
    EXPECT_NEAR(BmiCalculator::compute(70.0, 175.0), 22.857, 0.01);
    EXPECT_DOUBLE_EQ(BmiCalculator::compute(70.0, 0.0), 0.0);
    EXPECT_NEAR(SHealth::computeBmi(70.0, 175.0), 22.857, 0.01);
}

TEST(SHealthBmiTest, ClassifyBmiBoundaries) {
    EXPECT_EQ(BmiCalculator::classify(18.5), BmiCategory::Underweight);
    EXPECT_EQ(BmiCalculator::classify(18.5001), BmiCategory::Normal);
    EXPECT_EQ(BmiCalculator::classify(23.0), BmiCategory::Overweight);
    EXPECT_EQ(BmiCalculator::classify(25.0), BmiCategory::Obesity);
    EXPECT_EQ(SHealth::classifyBmi(25.0), BmiCategory::Obesity);
}

TEST(SHealthBmiTest, ImputeMissingWeightByDecade) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "1,22,100,160\n"
        "2,23,0,160\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 2);
    EXPECT_NEAR(shealth.getCategoryRatio(20, BmiCategory::Obesity), 100.0, 0.01);
}

TEST(SHealthBmiTest, ImputeMissingHeightByDecade) {
    std::vector<HealthRecord> records = {
        {1, 22, 70.0, 170.0, 0.0},
        {2, 23, 70.0, 0.0, 0.0},
    };

    DataImputer::imputeMissingValues(records);
    BmiCalculator::applyToAll(records);

    EXPECT_DOUBLE_EQ(records[1].heightCm, 170.0);
    EXPECT_NEAR(records[0].bmi, records[1].bmi, 0.001);
}

TEST(SHealthBmiTest, DecadeDistributionMatchesCategoryRatio) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "1,25,50,160\n"
        "2,26,90,160\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 2);

    const CategoryRatios distribution = shealth.getDecadeDistribution(20);
    EXPECT_EQ(distribution[categoryIndex(BmiCategory::Underweight)],
              shealth.getCategoryRatio(20, BmiCategory::Underweight));
    EXPECT_EQ(distribution[categoryIndex(BmiCategory::Obesity)],
              shealth.getCategoryRatio(20, BmiCategory::Obesity));
}

TEST(SHealthBmiTest, OverallCategoryRatio) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "1,25,50,160\n"
        "2,35,90,160\n"
        "3,45,70,175\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 3);

    const CategoryRatios overall = shealth.getOverallDistribution();
    double sum = 0.0;
    for (double ratio : overall) {
        sum += ratio;
    }
    EXPECT_NEAR(sum, 100.0, 0.01);
    EXPECT_EQ(shealth.getOverallCategoryRatio(BmiCategory::Underweight),
              overall[categoryIndex(BmiCategory::Underweight)]);
}

TEST(SHealthBmiTest, NormalBmiUserIds) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "101,25,60,170\n"
        "102,26,90,160\n"
        "103,27,50,180\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 3);

    const std::vector<int> normalIds = shealth.getNormalBmiUserIds();
    ASSERT_EQ(normalIds.size(), 1u);
    EXPECT_EQ(normalIds[0], 101);
}

TEST(SHealthBmiTest, LegacyTypeCodeCompatibility) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "1,25,50,160\n"
        "2,26,90,160\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 2);
    EXPECT_EQ(shealth.getBmiRatio(20, LegacyTypeCode::kUnderweight),
              shealth.getCategoryRatio(20, BmiCategory::Underweight));
}

TEST(SHealthBmiTest, InvalidTypeCodeReturnsZero) {
    EXPECT_FALSE(categoryFromTypeCode(999).has_value());

    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "1,25,60,170\n");

    SHealth shealth;
    ASSERT_EQ(shealth.processFile(path), 1);
    EXPECT_DOUBLE_EQ(shealth.getBmiRatio(20, 999), 0.0);
}

TEST(SHealthBmiTest, SkipsInvalidRows) {
    const std::string path = writeTempCsv(
        "id,age,weight,height\n"
        "\n"
        "1,25,60,170\n"
        "bad-row\n"
        "2,26,70,175\n");

    SHealth shealth;
    EXPECT_EQ(shealth.processFile(path), 2);
}
