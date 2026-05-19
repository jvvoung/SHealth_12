#include "BmiCalculator.h"

double BmiCalculator::compute(double weightKg, double heightCm) {
    if (heightCm <= PhysicalUnits::kMissingValue) {
        return PhysicalUnits::kInvalidBmi;
    }
    const double heightM = heightCm / PhysicalUnits::kCentimetersPerMeter;
    return weightKg / (heightM * heightM);
}

BmiCategory BmiCalculator::classify(double bmi) {
    if (bmi <= BmiThresholds::kUnderweightMax) {
        return BmiCategory::Underweight;
    }
    if (bmi < BmiThresholds::kNormalMax) {
        return BmiCategory::Normal;
    }
    if (bmi < BmiThresholds::kOverweightMax) {
        return BmiCategory::Overweight;
    }
    return BmiCategory::Obesity;
}

void BmiCalculator::applyToAll(std::vector<HealthRecord>& records) {
    for (HealthRecord& record : records) {
        record.bmi = compute(record.weightKg, record.heightCm);
    }
}
