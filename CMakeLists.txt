cmake_minimum_required(VERSION 3.24)
project(bond_analytics VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<1:bin>")
include(CTest)
include(FetchContent)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
option(BOND_BUILD_BENCHMARKS "Build pricing algorithm benchmarks" ON)
option(BOND_BUILD_DOCS "Provide checked Doxygen HTML documentation" ON)

add_library(bond_core src/analytics.cpp)
target_include_directories(bond_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
    target_compile_options(bond_core PRIVATE /W4)
else()
    target_compile_options(bond_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(bond_analytics src/main.cpp)
target_link_libraries(bond_analytics PRIVATE bond_core)

if(BUILD_TESTING)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL "https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz"
        URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(doctest)
    add_executable(bond_tests tests/test_analytics.cpp)
    target_link_libraries(bond_tests PRIVATE bond_core doctest::doctest)
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
    doctest_discover_tests(bond_tests)
    add_test(NAME cli_decimal_oracle
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/check_cli.py"
                --app "$<TARGET_FILE:bond_analytics>")
    set_tests_properties(cli_decimal_oracle PROPERTIES TIMEOUT 45)
    add_test(NAME workflow_evidence_checks
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_workflow_tools.py")
    set_tests_properties(workflow_evidence_checks PROPERTIES TIMEOUT 30)
endif()

if(BOND_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
        URL "https://github.com/google/benchmark/archive/refs/tags/v1.9.5.tar.gz"
        URL_HASH SHA256=9631341c82bac4a288bef951f8b26b41f69021794184ece969f8473977eaa340
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(benchmark)
    add_executable(bond_benchmark benchmarks/benchmark_pricing.cpp)
    target_link_libraries(bond_benchmark PRIVATE bond_core benchmark::benchmark)
    add_custom_target(run_benchmarks
        COMMAND "$<TARGET_FILE:bond_benchmark>" --benchmark_min_time=0.02s
                --benchmark_repetitions=3
                "--benchmark_out=${CMAKE_BINARY_DIR}/benchmark.json"
                --benchmark_out_format=json
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_benchmark.py"
                "${CMAKE_BINARY_DIR}/benchmark.json"
        DEPENDS bond_benchmark
        USES_TERMINAL VERBATIM
        COMMENT "Compare direct discounting and Horner pricing; validate JSON")
endif()

if(BOND_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    configure_file(Doxyfile.in "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_docs.py"
                "${CMAKE_BINARY_DIR}/docs/html"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL VERBATIM
        COMMENT "Generate Doxygen HTML and check local files and anchors")
endif()
