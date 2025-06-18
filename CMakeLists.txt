cmake_minimum_required(VERSION 3.14)
project(mpesa-cpp 
    VERSION 0.1.0
    DESCRIPTION "M-Pesa C++ SDK"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags for C++17
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-std=c++17 -fPIC)
endif()

# Use pkg-config to find libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)
find_package(GTest REQUIRED)

# Library target
add_library(${PROJECT_NAME} STATIC
    src/mpesa/auth.cpp
    src/mpesa/stk/client.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CURL_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_directories(${PROJECT_NAME} PUBLIC
    ${CURL_LIBRARY_DIRS}
    ${OPENSSL_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CURL_LIBRARIES}
        ssl
        crypto
)

option(MPESA_BUILD_TESTS "Build tests" ON) # Default set to ON for contributors

if (MPESA_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")

    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Building tests")
endif()

# Documentation setup
if(MPESA_BUILD_DOCS)
    find_package(Doxygen REQUIRED COMPONENTS doxygen dot)
    add_subdirectory(docs)
endif()

# Conditionally include examples directory if it exists
if (EXISTS "${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()