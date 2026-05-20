#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#if defined(_WIN32)
#include <io.h>
#include <stdlib.h>
#define SHEALTH_POPEN _popen
#define SHEALTH_PCLOSE _pclose
#else
#include <cstdio>
#define SHEALTH_POPEN popen
#define SHEALTH_PCLOSE pclose
#endif

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "C++17 filesystem support is required for golden master tests"
#endif

#ifndef SHEALTH_ENABLE_GOLDEN_TESTS
#define SHEALTH_ENABLE_GOLDEN_TESTS 1
#endif

#ifndef SHEALTH_SOURCE_DIR
#define SHEALTH_SOURCE_DIR "."
#endif

#ifndef SHEALTH_GOLDEN_FILE
#define SHEALTH_GOLDEN_FILE "test/golden/shealth_bmi_report.golden.txt"
#endif

#ifndef SHEALTH_GOLDEN_ACTUAL_DIR
#define SHEALTH_GOLDEN_ACTUAL_DIR "test/golden/actual"
#endif

#ifndef SHEALTH_BMI_EXE
#define SHEALTH_BMI_EXE "SHealthBMI"
#endif

namespace {

bool envFlagEnabled(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return false;
    }
    const bool enabled =
        value[0] == '1' || value[0] == 't' || value[0] == 'T' || value[0] == 'y' ||
        value[0] == 'Y';
    free(value);
    return enabled;
#else
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return false;
    }
    return value[0] == '1' || value[0] == 't' || value[0] == 'T' || value[0] == 'y' ||
           value[0] == 'Y';
#endif
}

std::string readFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << content;
    return static_cast<bool>(output);
}

std::string normalizeLineEndings(std::string text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            normalized.push_back('\n');
            continue;
        }
        normalized.push_back(text[i]);
    }
    return normalized;
}

std::string trimTrailingWhitespace(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == ' ' || text.back() == '\t')) {
        text.pop_back();
    }
    return text;
}

std::string normalizeOutput(std::string text) {
    return trimTrailingWhitespace(normalizeLineEndings(std::move(text)));
}

std::string runCaptureStdout(const fs::path& executablePath) {
    const std::string command = "\"" + executablePath.string() + "\"";

    FILE* pipe = SHEALTH_POPEN(command.c_str(), "r");
    if (pipe == nullptr) {
        return {};
    }

    std::string captured;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        captured += buffer;
    }
    SHEALTH_PCLOSE(pipe);
    return captured;
}

fs::path sourceDir() { return fs::path(SHEALTH_SOURCE_DIR); }

fs::path goldenPath() { return sourceDir() / SHEALTH_GOLDEN_FILE; }

fs::path actualPath() {
    return sourceDir() / SHEALTH_GOLDEN_ACTUAL_DIR / "shealth_bmi_report.actual.txt";
}

fs::path shealthDatPath() { return sourceDir() / "shealth.dat"; }

}  // namespace

TEST(GoldenMasterTest, ShealthDatReportMatchesExpected) {
#if !SHEALTH_ENABLE_GOLDEN_TESTS
    GTEST_SKIP() << "Golden master tests are disabled (SHEALTH_ENABLE_GOLDEN_TESTS=OFF)";
#endif

    const fs::path dataFile = shealthDatPath();
    ASSERT_TRUE(fs::exists(dataFile))
        << "Missing shealth.dat at " << dataFile.string();

    const fs::path executable = fs::path(SHEALTH_BMI_EXE);
    ASSERT_TRUE(fs::exists(executable))
        << "Missing SHealthBMI executable at " << executable.string();

    const fs::path previousCwd = fs::current_path();
    fs::current_path(sourceDir());

    const std::string actualRaw = runCaptureStdout(executable);
    fs::current_path(previousCwd);

    ASSERT_FALSE(actualRaw.empty()) << "SHealthBMI produced no stdout";

    const std::string actual = normalizeOutput(actualRaw);
    const fs::path goldenFile = goldenPath();

    if (envFlagEnabled("UPDATE_GOLDEN")) {
        ASSERT_TRUE(writeFile(goldenFile, actual))
            << "Failed to write golden file: " << goldenFile.string();
        GTEST_SKIP() << "Golden file updated at " << goldenFile.string()
                     << " (UPDATE_GOLDEN=1). Re-run without UPDATE_GOLDEN to verify.";
    }

    const std::string expected = normalizeOutput(readFile(goldenFile));
    ASSERT_FALSE(expected.empty())
        << "Missing or empty golden file: " << goldenFile.string()
        << ". Run: cmake --build build --target update-golden";

    if (actual != expected) {
        writeFile(actualPath(), actual);
        FAIL() << "Golden master mismatch.\n"
               << "  expected: " << goldenFile.string() << "\n"
               << "  actual:   " << actualPath().string() << "\n"
               << "  To refresh: cmake --build build --target update-golden";
    }
}
