cmake_minimum_required(VERSION 3.14)
project(scalatrix_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module
include(FetchContent)

# Declare Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)

# Make Catch2 available
FetchContent_MakeAvailable(Catch2)


# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Common source files
set(SCALATRIX_SOURCES
    ${CMAKE_SOURCE_DIR}/src/params.cpp
    ${CMAKE_SOURCE_DIR}/src/affine_transform.cpp
    ${CMAKE_SOURCE_DIR}/src/linear_solver.cpp
    ${CMAKE_SOURCE_DIR}/src/scale.cpp
    ${CMAKE_SOURCE_DIR}/src/mos.cpp
    ${CMAKE_SOURCE_DIR}/src/pitchset.cpp
    ${CMAKE_SOURCE_DIR}/src/lattice.cpp
    ${CMAKE_SOURCE_DIR}/src/label_calculator.cpp
    ${CMAKE_SOURCE_DIR}/src/node.cpp
)

# Test executables
add_executable(test_affine_transform
    test_affine_transform.cpp
    ${CMAKE_SOURCE_DIR}/src/params.cpp
    ${CMAKE_SOURCE_DIR}/src/affine_transform.cpp
    ${CMAKE_SOURCE_DIR}/src/linear_solver.cpp
)

add_executable(test_scale
    test_scale.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_mos
    test_mos.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_pitch_sets
    test_pitch_sets.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_label_calculator
    test_label_calculator.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_integration
    test_integration.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_reference_paths
    test_reference_paths.cpp
    ${SCALATRIX_SOURCES}
)

add_executable(test_node
    test_node.cpp
    ${SCALATRIX_SOURCES}
)

# Link libraries
target_link_libraries(test_affine_transform Catch2::Catch2WithMain)
target_link_libraries(test_scale Catch2::Catch2WithMain)
target_link_libraries(test_mos Catch2::Catch2WithMain)
target_link_libraries(test_pitch_sets Catch2::Catch2WithMain)
target_link_libraries(test_label_calculator Catch2::Catch2WithMain)
target_link_libraries(test_integration Catch2::Catch2WithMain)
target_link_libraries(test_reference_paths Catch2::Catch2WithMain)
target_link_libraries(test_node Catch2::Catch2WithMain)

# Enable testing
include(CTest)
include(Catch)
catch_discover_tests(test_affine_transform)
catch_discover_tests(test_scale)
catch_discover_tests(test_mos)
catch_discover_tests(test_pitch_sets)
catch_discover_tests(test_label_calculator)
catch_discover_tests(test_integration)
catch_discover_tests(test_reference_paths)
catch_discover_tests(test_node)