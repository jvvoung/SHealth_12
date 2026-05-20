#include "SHealth.h"

#include <iostream>
#include <optional>

#include "BmiCalculator.h"
#include "DataImputer.h"

int SHealth::processFile(const std::string& filename) {
    records_.clear();

    if (!csvReader_.load(filename, records_)) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return -1;
    }

    DataImputer::imputeMissingValues(records_);
    BmiCalculator::applyToAll(records_);
    statistics_.compute(records_);

    return static_cast<int>(records_.size());
}

double SHealth::getCategoryRatio(int ageDecade, BmiCategory category) const {
    return statistics_.getDecadeRatio(ageDecade, category);
}

double SHealth::getBmiRatio(int ageDecade, int typeCode) const {
    const std::optional<BmiCategory> category = categoryFromTypeCode(typeCode);
    if (!category.has_value()) {
        return 0.0;
    }
    return getCategoryRatio(ageDecade, category.value());
}

CategoryRatios SHealth::getDecadeDistribution(int ageDecade) const {
    return statistics_.getDecadeDistribution(ageDecade);
}

CategoryRatios SHealth::getOverallDistribution() const {
    return statistics_.getOverallDistribution();
}

double SHealth::getOverallCategoryRatio(BmiCategory category) const {
    return statistics_.getOverallRatio(category);
}

std::vector<int> SHealth::getNormalBmiUserIds() const {
    return statistics_.getNormalBmiUserIds();
}

double SHealth::computeBmi(double weightKg, double heightCm) {
    return BmiCalculator::compute(weightKg, heightCm);
}

BmiCategory SHealth::classifyBmi(double bmi) {
    return BmiCalculator::classify(bmi);
}

int SHealth::ageDecadeStart(int age) {
    return AgeDecade::startForAge(age);
}

bool SHealth::isInAgeDecade(int age, int decadeStart) {
    return AgeDecade::contains(age, decadeStart);
}
