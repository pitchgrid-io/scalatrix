cmake_minimum_required(VERSION 3.10)
project(scalatrix)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure proper UTF-8 handling on Windows
if(WIN32)
    add_compile_options(/utf-8)
endif()

# Source files
set(SOURCES
    src/affine_transform.cpp
    src/scale.cpp
    src/lattice.cpp
    src/params.cpp
    src/mos.cpp
    src/pitchset.cpp
    src/linear_solver.cpp
    src/label_calculator.cpp
    src/node.cpp
    src/main.cpp
)

# Main library target
add_library(scalatrix STATIC ${SOURCES})
target_include_directories(scalatrix PUBLIC include)

# Build options
option(BUILD_WASM "Build WebAssembly target" OFF)
option(BUILD_PYTHON "Build Python bindings" OFF)
option(BUILD_IOS "Build iOS target" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" ON)

# Native example executable
if(BUILD_EXAMPLES AND NOT EMSCRIPTEN)
    add_executable(scalatrix_example examples/native/native_example.cpp)
    target_link_libraries(scalatrix_example PRIVATE scalatrix)
endif()

# WebAssembly build
if(BUILD_WASM OR EMSCRIPTEN)
    add_executable(scalatrix_wasm ${SOURCES})
    target_include_directories(scalatrix_wasm PUBLIC include)
    target_link_options(scalatrix_wasm PRIVATE
        --emit-tsd "$<TARGET_FILE_DIR:scalatrix_wasm>/scalatrix.d.ts"
    )
    set_target_properties(scalatrix_wasm PROPERTIES
        OUTPUT_NAME "scalatrix"
        SUFFIX ".js"
        LINK_FLAGS "--bind -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME='Scalatrix' -s USE_WEBGL2=1 -s EXPORTED_RUNTIME_METHODS='[ccall, cwrap]' -s SINGLE_FILE=1"
    )
endif()

# Python bindings
if(BUILD_PYTHON)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    find_package(pybind11 REQUIRED)

    add_library(scalatrix_python MODULE ${SOURCES} src/python_bindings.cpp)
    target_include_directories(scalatrix_python PUBLIC include)
    target_link_libraries(scalatrix_python PRIVATE pybind11::pybind11 Python3::Python)

    # Set platform-appropriate suffix for Python extension module
    if(WIN32)
        set(PYTHON_MODULE_SUFFIX ".pyd")
    else()
        set(PYTHON_MODULE_SUFFIX ".so")
    endif()

    set_target_properties(scalatrix_python PROPERTIES
        OUTPUT_NAME "scalatrix"
        PREFIX ""
        SUFFIX "${PYTHON_MODULE_SUFFIX}"
    )

    # Install to root for scikit-build-core packaging
    # scikit-build-core will handle placing it in site-packages
    install(TARGETS scalatrix_python
        LIBRARY DESTINATION .
    )
endif()

# iOS build
if(BUILD_IOS)
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    
    add_library(scalatrix_ios STATIC ${SOURCES})
    target_include_directories(scalatrix_ios PUBLIC include)
    set_target_properties(scalatrix_ios PROPERTIES
        XCODE_ATTRIBUTE_ENABLE_BITCODE "YES"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "12.0"
    )
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()