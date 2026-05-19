#pragma once

#include <string>
#include <vector>

#include "BmiStatistics.h"
#include "BmiTypes.h"
#include "CsvReader.h"

class SHealth {
public:
    static constexpr int kMinAgeDecade = AgeDecade::kMin;
    static constexpr int kMaxAgeDecade = AgeDecade::kMax;
    static constexpr int kAgeDecadeStep = AgeDecade::kStep;
    static constexpr int kDecadeCount = AgeDecade::kCount;
    static constexpr int kCategoryCount = BmiCategoryMeta::kCount;

    static constexpr int kTypeUnderweight = LegacyTypeCode::kUnderweight;
    static constexpr int kTypeNormal = LegacyTypeCode::kNormal;
    static constexpr int kTypeOverweight = LegacyTypeCode::kOverweight;
    static constexpr int kTypeObesity = LegacyTypeCode::kObesity;

    int processFile(const std::string& filename);

    double getCategoryRatio(int ageDecade, BmiCategory category) const;
    double getBmiRatio(int ageDecade, int typeCode) const;

    CategoryRatios getDecadeDistribution(int ageDecade) const;
    CategoryRatios getOverallDistribution() const;
    double getOverallCategoryRatio(BmiCategory category) const;
    std::vector<int> getNormalBmiUserIds() const;

    const std::vector<HealthRecord>& records() const { return records_; }

    static double computeBmi(double weightKg, double heightCm);
    static BmiCategory classifyBmi(double bmi);
    static int ageDecadeStart(int age);
    static bool isInAgeDecade(int age, int decadeStart);

private:
    std::vector<HealthRecord> records_;
    BmiStatistics statistics_;
    CsvReader csvReader_;
};
