#pragma once

#include <string>
#include <vector>

#include "BmiTypes.h"

class CsvReader {
public:
    enum class Column { Id = 0, Age = 1, Weight = 2, Height = 3 };
    static constexpr size_t kColumnCount = 4;

    bool load(const std::string& filename, std::vector<HealthRecord>& out) const;

private:
    static std::vector<std::string> split(const std::string& line, char delimiter);
};
