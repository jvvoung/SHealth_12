#pragma once

#include "BmiTypes.h"

class BmiCalculator {
public:
    static double compute(double weightKg, double heightCm);
    static BmiCategory classify(double bmi);
    static void applyToAll(std::vector<HealthRecord>& records);
};
