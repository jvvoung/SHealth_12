#include "CsvReader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

bool CsvReader::load(const std::string& filename, std::vector<HealthRecord>& out) const {
    out.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::getline(file, line);  // header

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> tokens = split(line, ',');
        if (tokens.size() < kColumnCount) {
            continue;
        }

        try {
            HealthRecord record;
            record.id = std::stoi(tokens[static_cast<size_t>(Column::Id)]);
            record.age = std::stoi(tokens[static_cast<size_t>(Column::Age)]);
            record.weightKg = std::stod(tokens[static_cast<size_t>(Column::Weight)]);
            record.heightCm = std::stod(tokens[static_cast<size_t>(Column::Height)]);
            out.push_back(record);
        } catch (const std::exception&) {
            continue;
        }
    }

    return true;
}

std::vector<std::string> CsvReader::split(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
