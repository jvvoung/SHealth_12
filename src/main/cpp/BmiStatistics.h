#pragma once

#include <vector>

#include "BmiTypes.h"

class BmiStatistics {
public:
    void compute(const std::vector<HealthRecord>& records);

    double getDecadeRatio(int ageDecade, BmiCategory category) const;
    double getOverallRatio(BmiCategory category) const;
    CategoryRatios getDecadeDistribution(int ageDecade) const;
    CategoryRatios getOverallDistribution() const;
    std::vector<int> getNormalBmiUserIds() const;

private:
    std::array<CategoryRatios, AgeDecade::kCount> decadeRatios_{};
    CategoryRatios overallRatios_{};
    std::vector<int> normalBmiUserIds_;

    static CategoryRatios buildRatios(const std::array<int, BmiCategoryMeta::kCount>& counts,
                                      int total);
};
