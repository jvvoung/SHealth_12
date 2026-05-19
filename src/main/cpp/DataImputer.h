#pragma once

#include "BmiTypes.h"

class DataImputer {
public:
    static void imputeMissingValues(std::vector<HealthRecord>& records);
};
