#include "DataImputer.h"

namespace {
using FieldMember = double HealthRecord::*;

void imputeFieldByAgeDecade(std::vector<HealthRecord>& records, FieldMember field) {
    for (int decade = AgeDecade::kMin; decade <= AgeDecade::kMax; decade += AgeDecade::kStep) {
        double sum = 0.0;
        int validCount = 0;

        for (const HealthRecord& record : records) {
            if (!AgeDecade::contains(record.age, decade)) {
                continue;
            }
            const double value = record.*field;
            if (value == PhysicalUnits::kMissingValue) {
                continue;
            }
            sum += value;
            validCount++;
        }

        if (validCount == 0) {
            continue;
        }

        const double average = sum / validCount;
        for (HealthRecord& record : records) {
            if (AgeDecade::contains(record.age, decade) &&
                record.*field == PhysicalUnits::kMissingValue) {
                record.*field = average;
            }
        }
    }
}
}  // namespace

void DataImputer::imputeMissingValues(std::vector<HealthRecord>& records) {
    imputeFieldByAgeDecade(records, &HealthRecord::weightKg);
    imputeFieldByAgeDecade(records, &HealthRecord::heightCm);
}
