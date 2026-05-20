#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

enum class BmiCategory { Underweight, Normal, Overweight, Obesity };

namespace BmiThresholds {
constexpr double kUnderweightMax = 18.5;
constexpr double kNormalMax = 23.0;
constexpr double kOverweightMax = 25.0;
}  // namespace BmiThresholds

namespace BmiCategoryMeta {
constexpr int kCount = 4;

std::array<BmiCategory, kCount> allCategories();
const char* categoryName(BmiCategory category);
}  // namespace BmiCategoryMeta

namespace LegacyTypeCode {
constexpr int kUnderweight = 100;
constexpr int kNormal = 200;
constexpr int kOverweight = 300;
constexpr int kObesity = 400;
}  // namespace LegacyTypeCode

namespace PhysicalUnits {
constexpr double kCentimetersPerMeter = 100.0;
constexpr double kMissingValue = 0.0;
constexpr double kInvalidBmi = 0.0;
}  // namespace PhysicalUnits

namespace StatisticsScale {
constexpr double kPercent = 100.0;
}  // namespace StatisticsScale

namespace AgeDecade {
constexpr int kMin = 20;
constexpr int kMax = 70;
constexpr int kStep = 10;
constexpr int kCount = (kMax - kMin) / kStep + 1;

int startForAge(int age);
bool contains(int age, int decadeStart);
int indexForDecade(int decadeStart);
}  // namespace AgeDecade

struct HealthRecord {
    int id = 0;
    int age = 0;
    double weightKg = 0.0;
    double heightCm = 0.0;
    double bmi = 0.0;
};

using CategoryRatios = std::array<double, BmiCategoryMeta::kCount>;

inline int categoryIndex(BmiCategory category) {
    return static_cast<int>(category);
}

std::optional<BmiCategory> categoryFromTypeCode(int typeCode);
