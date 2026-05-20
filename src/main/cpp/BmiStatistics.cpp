#include "BmiStatistics.h"

#include "BmiCalculator.h"

CategoryRatios BmiStatistics::buildRatios(
    const std::array<int, BmiCategoryMeta::kCount>& counts, int total) {
    CategoryRatios ratios{};
    if (total <= 0) {
        return ratios;
    }

    for (size_t i = 0; i < BmiCategoryMeta::kCount; ++i) {
        ratios[i] = static_cast<double>(counts[i]) * StatisticsScale::kPercent / total;
    }
    return ratios;
}

void BmiStatistics::compute(const std::vector<HealthRecord>& records) {
    decadeRatios_ = {};
    overallRatios_ = {};
    normalBmiUserIds_.clear();

    std::array<std::array<int, BmiCategoryMeta::kCount>, AgeDecade::kCount> decadeCounts{};
    std::array<int, AgeDecade::kCount> decadeTotals{};
    std::array<int, BmiCategoryMeta::kCount> overallCounts{};
    int overallTotal = 0;

    for (const HealthRecord& record : records) {
        if (record.bmi <= PhysicalUnits::kInvalidBmi) {
            continue;
        }

        const BmiCategory category = BmiCalculator::classify(record.bmi);
        const size_t categoryIdx = static_cast<size_t>(categoryIndex(category));

        overallCounts[categoryIdx]++;
        overallTotal++;

        if (category == BmiCategory::Normal) {
            normalBmiUserIds_.push_back(record.id);
        }

        const int decadeStart = AgeDecade::startForAge(record.age);
        const int decadeIdx = AgeDecade::indexForDecade(decadeStart);
        if (decadeIdx < 0 || !AgeDecade::contains(record.age, decadeStart)) {
            continue;
        }

        decadeCounts[static_cast<size_t>(decadeIdx)][categoryIdx]++;
        decadeTotals[static_cast<size_t>(decadeIdx)]++;
    }

    for (size_t decadeIdx = 0; decadeIdx < AgeDecade::kCount; ++decadeIdx) {
        if (decadeTotals[decadeIdx] > 0) {
            decadeRatios_[decadeIdx] =
                buildRatios(decadeCounts[decadeIdx], decadeTotals[decadeIdx]);
        }
    }

    overallRatios_ = buildRatios(overallCounts, overallTotal);
}

double BmiStatistics::getDecadeRatio(int ageDecade, BmiCategory category) const {
    const int decadeIdx = AgeDecade::indexForDecade(ageDecade);
    if (decadeIdx < 0) {
        return 0.0;
    }
    return decadeRatios_[static_cast<size_t>(decadeIdx)]
                        [static_cast<size_t>(categoryIndex(category))];
}

double BmiStatistics::getOverallRatio(BmiCategory category) const {
    return overallRatios_[static_cast<size_t>(categoryIndex(category))];
}

CategoryRatios BmiStatistics::getDecadeDistribution(int ageDecade) const {
    const int decadeIdx = AgeDecade::indexForDecade(ageDecade);
    if (decadeIdx < 0) {
        return {};
    }
    return decadeRatios_[static_cast<size_t>(decadeIdx)];
}

CategoryRatios BmiStatistics::getOverallDistribution() const {
    return overallRatios_;
}

std::vector<int> BmiStatistics::getNormalBmiUserIds() const {
    return normalBmiUserIds_;
}
