#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "BmiCalculator.h"
#include "BmiStatistics.h"
#include "CsvReader.h"
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

// ---------------------------------------------------------------------------
// Existing 10 tests (do not rename or remove)
// ---------------------------------------------------------------------------

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
    // Given: one valid height and one height=0 missing in 20s decade
    // When:  DataImputer::imputeMissingValues then BmiCalculator::applyToAll
    // Then:  missing height replaced by decade average (170.0), BMI matches peer
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
    // Given: CSV with one Underweight and one Obesity in 20s decade
    // When:  processFile then getDecadeDistribution(20)
    // Then:  distribution array matches getCategoryRatio(20, category)
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
    // Given: 3 users (Underweight, Obesity, Normal) across decades
    // When:  processFile then getOverallDistribution
    // Then:  four categories sum to ~100%, getOverallCategoryRatio matches array
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
    // Given: one Normal (18.5<BMI<23), one Obesity, one Underweight
    // When:  processFile then getNormalBmiUserIds
    // Then:  single Normal user id returned in processing order
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

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

class CsvProcessFixture : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_shealth_" + std::to_string(++counter_) + ".csv";
    }

    void TearDown() override { std::remove(path_.c_str()); }

    std::string writeCsv(const std::string& content) {
        std::ofstream file(path_);
        file << content;
        return path_;
    }

    int processBody(SHealth& shealth, const std::string& body) {
        writeCsv("id,age,weight,height\n" + body);
        return shealth.processFile(path_);
    }

    std::string path_;
    static inline int counter_{0};
};

class RecordsPipelineFixture : public ::testing::Test {
protected:
    void runPipeline() {
        DataImputer::imputeMissingValues(records_);
        BmiCalculator::applyToAll(records_);
        statistics_.compute(records_);
    }

    std::vector<HealthRecord> records_;
    BmiStatistics statistics_;
};

// ---------------------------------------------------------------------------
// BmiCalculatorTest — TC-13, TC-23
// ---------------------------------------------------------------------------

class BmiCalculatorTest : public ::testing::Test {};

TEST_F(BmiCalculatorTest, Classify_NormalUpperBound_22_99) {
    // Given: BMI just below Normal upper bound (23.0)
    // When:  classify is called with 22.99
    // Then:  category is Normal (TC-13)
    EXPECT_EQ(BmiCalculator::classify(22.99), BmiCategory::Normal);
}

TEST_F(BmiCalculatorTest, Classify_OverweightUpperBound_24_99) {
    // Given: BMI just below Overweight upper bound (25.0)
    // When:  classify is called with 24.99
    // Then:  category is Overweight (TC-13)
    EXPECT_EQ(BmiCalculator::classify(24.99), BmiCategory::Overweight);
}

TEST_F(BmiCalculatorTest, Compute_NegativeHeight_ReturnsZero) {
    // Given: positive weight, non-positive height
    // When:  compute is called
    // Then:  BMI is 0.0 (invalid)
    EXPECT_DOUBLE_EQ(BmiCalculator::compute(70.0, -10.0), 0.0);
    EXPECT_DOUBLE_EQ(BmiCalculator::compute(70.0, 0.0), 0.0);
}

TEST_F(BmiCalculatorTest, ApplyToAll_SetsBmiOnEachRecord) {
    // Given: two records with known weight/height
    // When:  applyToAll runs
    // Then:  each record.bmi matches compute()
    std::vector<HealthRecord> records = {
        {1, 25, 70.0, 175.0, 0.0},
        {2, 30, 80.0, 180.0, 0.0},
    };
    BmiCalculator::applyToAll(records);
    EXPECT_NEAR(records[0].bmi, BmiCalculator::compute(70.0, 175.0), 0.01);
    EXPECT_NEAR(records[1].bmi, BmiCalculator::compute(80.0, 180.0), 0.01);
}

TEST_F(BmiCalculatorTest, Classify_ZeroBmi_IsUnderweight) {
    // Given: BMI 0 (invalid for statistics)
    // When:  classify is called
    // Then:  classification is Underweight (statistics exclude separately)
    EXPECT_EQ(BmiCalculator::classify(0.0), BmiCategory::Underweight);
}

// ---------------------------------------------------------------------------
// DataImputerTest — TC-16, TC-17
// ---------------------------------------------------------------------------

class DataImputerTest : public RecordsPipelineFixture {};

TEST_F(DataImputerTest, AllZeroWeightInDecade_NoImpute) {
    // Given: 20대 전원 weight=0
    // When:  impute + applyToAll
    // Then:  weight stays 0, bmi=0 (TC-16)
    records_ = {
        {1, 22, 0.0, 170.0, 0.0},
        {2, 23, 0.0, 175.0, 0.0},
    };
    DataImputer::imputeMissingValues(records_);
    BmiCalculator::applyToAll(records_);
    EXPECT_DOUBLE_EQ(records_[0].weightKg, 0.0);
    EXPECT_DOUBLE_EQ(records_[1].weightKg, 0.0);
    EXPECT_DOUBLE_EQ(records_[0].bmi, 0.0);
    EXPECT_DOUBLE_EQ(records_[1].bmi, 0.0);
}

TEST_F(DataImputerTest, BothZeroWeightThenHeight_Imputed) {
    // Given: peer with valid weight/height, one record with w=0,h=0
    // When:  impute (weight first, then height) + applyToAll
    // Then:  both fields imputed, bmi>0 (TC-17)
    records_ = {
        {1, 22, 70.0, 170.0, 0.0},
        {2, 23, 0.0, 0.0, 0.0},
    };
    DataImputer::imputeMissingValues(records_);
    BmiCalculator::applyToAll(records_);
    EXPECT_GT(records_[1].weightKg, 0.0);
    EXPECT_GT(records_[1].heightCm, 0.0);
    EXPECT_GT(records_[1].bmi, 0.0);
    EXPECT_NEAR(records_[0].bmi, records_[1].bmi, 0.001);
}

TEST_F(DataImputerTest, ValidCountZero_SkipsDecade) {
    // Given: 30대 only, all weight=0 (no valid peer)
    // When:  imputeMissingValues
    // Then:  weight remains 0
    records_ = {
        {1, 32, 0.0, 170.0, 0.0},
        {2, 35, 0.0, 175.0, 0.0},
    };
    DataImputer::imputeMissingValues(records_);
    EXPECT_DOUBLE_EQ(records_[0].weightKg, 0.0);
    EXPECT_DOUBLE_EQ(records_[1].weightKg, 0.0);
}

TEST_F(DataImputerTest, ImputeHeight_ByDecadeAverage) {
    // Given: one valid height in 20대, one missing
    // When:  imputeMissingValues
    // Then:  missing height equals decade average
    records_ = {
        {1, 22, 70.0, 168.0, 0.0},
        {2, 25, 70.0, 0.0, 0.0},
    };
    DataImputer::imputeMissingValues(records_);
    EXPECT_DOUBLE_EQ(records_[1].heightCm, 168.0);
}

// ---------------------------------------------------------------------------
// CsvReaderTest — TC-21
// ---------------------------------------------------------------------------

class CsvReaderTest : public CsvProcessFixture {};

TEST_F(CsvReaderTest, Load_MissingFile_ReturnsFalse) {
    // Given: non-existent path
    // When:  CsvReader::load
    // Then:  returns false, out empty
    CsvReader reader;
    std::vector<HealthRecord> records;
    EXPECT_FALSE(reader.load("no_such_shealth_file.csv", records));
    EXPECT_TRUE(records.empty());
}

TEST_F(CsvReaderTest, HeaderOnly_ReturnsEmpty) {
    // Given: CSV with header only
    // When:  load
    // Then:  zero records (TC-21)
    writeCsv("id,age,weight,height\n");
    CsvReader reader;
    std::vector<HealthRecord> records;
    ASSERT_TRUE(reader.load(path_, records));
    EXPECT_TRUE(records.empty());
}

TEST_F(CsvReaderTest, ThreeColumns_Skipped) {
    // Given: row with only 3 columns
    // When:  load
    // Then:  row skipped (TC-21)
    writeCsv(
        "id,age,weight,height\n"
        "1,25,60\n"
        "2,26,70,175\n");
    CsvReader reader;
    std::vector<HealthRecord> records;
    ASSERT_TRUE(reader.load(path_, records));
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].id, 2);
}

TEST_F(CsvReaderTest, SkipsEmptyLineAndBadRow) {
    // Given: empty line and unparseable row
    // When:  load
    // Then:  only valid rows loaded
    writeCsv(
        "id,age,weight,height\n"
        "\n"
        "1,25,60,170\n"
        "bad-row\n"
        "2,26,70,175\n");
    CsvReader reader;
    std::vector<HealthRecord> records;
    ASSERT_TRUE(reader.load(path_, records));
    EXPECT_EQ(records.size(), 2u);
}

// ---------------------------------------------------------------------------
// BmiStatisticsTest — TC-14, TC-15, TC-18, TC-19, TC-22
// ---------------------------------------------------------------------------

class BmiStatisticsTest : public RecordsPipelineFixture {};

TEST_F(BmiStatisticsTest, ExcludesNonPositiveBmi) {
    // Given: one bmi=0 (height≤0), one valid Normal
    // When:  statistics computed
    // Then:  only valid record counted; overall Normal 100% (TC-14)
    records_ = {
        {1, 25, 70.0, 0.0, 0.0},
        {2, 30, 60.0, 170.0, 0.0},
    };
    runPipeline();
    EXPECT_NEAR(statistics_.getOverallRatio(BmiCategory::Normal), 100.0, 0.01);
    const std::vector<int> normalIds = statistics_.getNormalBmiUserIds();
    ASSERT_EQ(normalIds.size(), 1u);
    EXPECT_EQ(normalIds[0], 2);
}

TEST_F(BmiStatisticsTest, Age19_ExcludedFromDecade_IncludedInOverall) {
    // Given: age 19 with valid BMI (Normal range)
    // When:  pipeline runs
    // Then:  20대 decade 0%, overall includes record (TC-15)
    records_ = {{1, 19, 60.0, 170.0, 0.0}};
    runPipeline();
    EXPECT_DOUBLE_EQ(statistics_.getDecadeRatio(20, BmiCategory::Normal), 0.0);
    EXPECT_NEAR(statistics_.getOverallRatio(BmiCategory::Normal), 100.0, 0.01);
}

TEST_F(BmiStatisticsTest, Age71_InSeventiesBucket) {
    // Given: age 71 (70≤age<80), valid BMI
    // When:  pipeline runs
    // Then:  70대 decade ratio > 0 (TC-15)
    records_ = {{1, 71, 60.0, 170.0, 0.0}};
    runPipeline();
    double sum = 0.0;
    const CategoryRatios dist = statistics_.getDecadeDistribution(70);
    for (double r : dist) {
        sum += r;
    }
    EXPECT_NEAR(sum, 100.0, 0.01);
}

TEST_F(BmiStatisticsTest, Age80_ExcludedFromDecade_IncludedInOverall) {
    // Given: age 80 (no valid decade index)
    // When:  pipeline runs
    // Then:  70대 decade 0%, overall 100% in one category (TC-15)
    records_ = {{1, 80, 60.0, 170.0, 0.0}};
    runPipeline();
    EXPECT_DOUBLE_EQ(statistics_.getDecadeRatio(70, BmiCategory::Normal), 0.0);
    EXPECT_NEAR(statistics_.getOverallRatio(BmiCategory::Normal), 100.0, 0.01);
}

TEST_F(BmiStatisticsTest, Age20And29_InTwentiesDecade) {
    // Given: ages 20 and 29 in same decade
    // When:  pipeline runs
    // Then:  both counted in 20대 (sum 100%)
    records_ = {
        {1, 20, 60.0, 170.0, 0.0},
        {2, 29, 90.0, 160.0, 0.0},
    };
    runPipeline();
    double sum = 0.0;
    const CategoryRatios dist = statistics_.getDecadeDistribution(20);
    for (double r : dist) {
        sum += r;
    }
    EXPECT_NEAR(sum, 100.0, 0.01);
}

TEST_F(BmiStatisticsTest, NormalBmiUserIds_EmptyAndMultiple) {
    // Given: pass 1 — Underweight only; pass 2 — two Normal + one Obesity
    // When:  statistics computed
    // Then:  sizes 0 and 2, IDs preserved (TC-18)
    records_ = {{1, 25, 45.0, 170.0, 0.0}};
    runPipeline();
    EXPECT_TRUE(statistics_.getNormalBmiUserIds().empty());

    records_ = {
        {10, 25, 60.0, 170.0, 0.0},
        {20, 26, 65.0, 175.0, 0.0},
        {30, 27, 90.0, 160.0, 0.0},
    };
    runPipeline();
    const std::vector<int> ids = statistics_.getNormalBmiUserIds();
    ASSERT_EQ(ids.size(), 2u);
    EXPECT_EQ(ids[0], 10);
    EXPECT_EQ(ids[1], 20);
}

TEST_F(BmiStatisticsTest, EmptyDecade_ReturnsZero) {
    // Given: data only in 30대
    // When:  query 20대 ratios
    // Then:  all zero; invalid decade 10 → 0 (TC-19)
    records_ = {{1, 35, 60.0, 170.0, 0.0}};
    runPipeline();
    EXPECT_DOUBLE_EQ(statistics_.getDecadeRatio(20, BmiCategory::Normal), 0.0);
    for (double r : statistics_.getDecadeDistribution(20)) {
        EXPECT_DOUBLE_EQ(r, 0.0);
    }
    EXPECT_DOUBLE_EQ(statistics_.getDecadeRatio(10, BmiCategory::Normal), 0.0);
}

TEST_F(BmiStatisticsTest, SingleRecord_OverallHundredPercent) {
    // Given: single valid record (45kg/170cm → Underweight)
    // When:  pipeline runs
    // Then:  one category 100%, sum ≈ 100% (TC-22)
    records_ = {{1, 25, 45.0, 170.0, 0.0}};
    runPipeline();
    EXPECT_NEAR(statistics_.getOverallRatio(BmiCategory::Underweight), 100.0, 0.01);
    double sum = 0.0;
    for (double r : statistics_.getOverallDistribution()) {
        sum += r;
    }
    EXPECT_NEAR(sum, 100.0, 0.01);
}

// ---------------------------------------------------------------------------
// SHealthProcessTest — TC-12, TC-20
// ---------------------------------------------------------------------------

class SHealthProcessTest : public CsvProcessFixture {};

TEST_F(SHealthProcessTest, ProcessFile_MissingFile_ReturnsMinusOne) {
    // Given: path that does not exist
    // When:  processFile
    // Then:  returns -1 (TC-12)
    SHealth shealth;
    EXPECT_EQ(shealth.processFile("no_such_shealth_process.csv"), -1);
    EXPECT_TRUE(shealth.records().empty());
}

TEST_F(SHealthProcessTest, ProcessFile_HeaderOnly_ReturnsZero) {
    // Given: header-only CSV
    // When:  processFile
    // Then:  returns 0, all ratios 0 (TC-21 integration)
    SHealth shealth;
    writeCsv("id,age,weight,height\n");
    EXPECT_EQ(shealth.processFile(path_), 0);
    EXPECT_DOUBLE_EQ(shealth.getOverallCategoryRatio(BmiCategory::Normal), 0.0);
}

TEST_F(SHealthProcessTest, CategoryFromTypeCode_AllLegacyCodes) {
    // Given: legacy type codes 100/200/300/400
    // When:  categoryFromTypeCode
    // Then:  each maps to expected BmiCategory (TC-20)
    ASSERT_TRUE(categoryFromTypeCode(100).has_value());
    EXPECT_EQ(categoryFromTypeCode(100).value(), BmiCategory::Underweight);
    ASSERT_TRUE(categoryFromTypeCode(200).has_value());
    EXPECT_EQ(categoryFromTypeCode(200).value(), BmiCategory::Normal);
    ASSERT_TRUE(categoryFromTypeCode(300).has_value());
    EXPECT_EQ(categoryFromTypeCode(300).value(), BmiCategory::Overweight);
    ASSERT_TRUE(categoryFromTypeCode(400).has_value());
    EXPECT_EQ(categoryFromTypeCode(400).value(), BmiCategory::Obesity);
    EXPECT_FALSE(categoryFromTypeCode(999).has_value());
}

TEST_F(SHealthProcessTest, GetBmiRatio_MatchesCategoryRatio_AllTypes) {
    // Given: 20대 Underweight + Obesity mix
    // When:  processFile + getBmiRatio per legacy code
    // Then:  getBmiRatio ≡ getCategoryRatio for 100~400 (TC-20)
    SHealth shealth;
    ASSERT_EQ(processBody(shealth,
                          "1,25,50,160\n"
                          "2,26,90,160\n"),
              2);
    EXPECT_EQ(shealth.getBmiRatio(20, LegacyTypeCode::kUnderweight),
              shealth.getCategoryRatio(20, BmiCategory::Underweight));
    EXPECT_EQ(shealth.getBmiRatio(20, LegacyTypeCode::kNormal),
              shealth.getCategoryRatio(20, BmiCategory::Normal));
    EXPECT_EQ(shealth.getBmiRatio(20, LegacyTypeCode::kOverweight),
              shealth.getCategoryRatio(20, BmiCategory::Overweight));
    EXPECT_EQ(shealth.getBmiRatio(20, LegacyTypeCode::kObesity),
              shealth.getCategoryRatio(20, BmiCategory::Obesity));
    EXPECT_DOUBLE_EQ(shealth.getBmiRatio(20, 999), 0.0);
}

TEST_F(SHealthProcessTest, DecadeVsOverall_Age19And71) {
    // Given: age 19 (decade excluded) and age 71 (70대)
    // When:  processFile
    // Then:  20대 empty for 19; 70대 has data for 71 (TC-15 integration)
    SHealth shealth;
    ASSERT_EQ(processBody(shealth,
                          "1,19,60,170\n"
                          "2,71,60,170\n"),
              2);
    EXPECT_DOUBLE_EQ(shealth.getCategoryRatio(20, BmiCategory::Normal), 0.0);
    EXPECT_NEAR(shealth.getCategoryRatio(70, BmiCategory::Normal), 100.0, 0.01);
    EXPECT_NEAR(shealth.getOverallCategoryRatio(BmiCategory::Normal), 100.0, 0.01);
}
