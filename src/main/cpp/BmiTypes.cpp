#include "BmiTypes.h"

std::array<BmiCategory, BmiCategoryMeta::kCount> BmiCategoryMeta::allCategories() {
    return {BmiCategory::Underweight, BmiCategory::Normal, BmiCategory::Overweight,
            BmiCategory::Obesity};
}

const char* BmiCategoryMeta::categoryName(BmiCategory category) {
    switch (category) {
        case BmiCategory::Underweight:
            return "underweight";
        case BmiCategory::Normal:
            return "normal";
        case BmiCategory::Overweight:
            return "overweight";
        case BmiCategory::Obesity:
            return "obesity";
    }
    return "unknown";
}

std::optional<BmiCategory> categoryFromTypeCode(int typeCode) {
    switch (typeCode) {
        case LegacyTypeCode::kUnderweight:
            return BmiCategory::Underweight;
        case LegacyTypeCode::kNormal:
            return BmiCategory::Normal;
        case LegacyTypeCode::kOverweight:
            return BmiCategory::Overweight;
        case LegacyTypeCode::kObesity:
            return BmiCategory::Obesity;
        default:
            return std::nullopt;
    }
}

int AgeDecade::startForAge(int age) {
    return (age / kStep) * kStep;
}

bool AgeDecade::contains(int age, int decadeStart) {
    return age >= decadeStart && age < decadeStart + kStep;
}

int AgeDecade::indexForDecade(int decadeStart) {
    if (decadeStart < kMin || decadeStart > kMax || (decadeStart - kMin) % kStep != 0) {
        return -1;
    }
    return (decadeStart - kMin) / kStep;
}
