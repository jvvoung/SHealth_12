cmake_minimum_required(VERSION 3.10)
project(CoreProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SHEALTH_ENABLE_GOLDEN_TESTS "Enable golden master regression tests (TC-25)" ON)

# Main library
add_library(shealth_lib
    src/main/cpp/BmiTypes.cpp
    src/main/cpp/CsvReader.cpp
    src/main/cpp/BmiCalculator.cpp
    src/main/cpp/DataImputer.cpp
    src/main/cpp/BmiStatistics.cpp
    src/main/cpp/SHealth.cpp
)
target_include_directories(shealth_lib PUBLIC src/main/cpp)

# Main executable
add_executable(SHealthBMI src/main/cpp/SHealthBMI.cpp)
target_link_libraries(SHealthBMI PRIVATE shealth_lib)

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executable
add_executable(SHealthBMITest
    test/cpp/SHealthBMITest.cpp
    test/cpp/GoldenMasterTest.cpp
)
target_link_libraries(SHealthBMITest PRIVATE shealth_lib GTest::gtest_main)

target_compile_definitions(SHealthBMITest PRIVATE
    SHEALTH_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    SHEALTH_GOLDEN_FILE="test/golden/shealth_bmi_report.golden.txt"
    SHEALTH_GOLDEN_ACTUAL_DIR="test/golden/actual"
    SHEALTH_BMI_EXE="$<TARGET_FILE:SHealthBMI>"
    SHEALTH_ENABLE_GOLDEN_TESTS=$<BOOL:${SHEALTH_ENABLE_GOLDEN_TESTS}>
)

add_test(
    NAME SHealthBMITest
    COMMAND $<TARGET_FILE:SHealthBMITest>
)
set_tests_properties(SHealthBMITest PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Regenerate golden file from SHealthBMI stdout (local / intentional changes only)
add_custom_target(update-golden
    COMMAND ${CMAKE_COMMAND} -E env UPDATE_GOLDEN=1
        $<TARGET_FILE:SHealthBMITest>
        --gtest_filter=GoldenMasterTest.ShealthDatReportMatchesExpected
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS SHealthBMITest SHealthBMI
    COMMENT "Regenerate test/golden/shealth_bmi_report.golden.txt from SHealthBMI stdout"
)
