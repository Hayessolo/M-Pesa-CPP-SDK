cmake_minimum_required(VERSION 3.10)
project(AuthExample)

# Set C++ standard for this example
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags for C++17
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-std=c++17)
endif()

# Use pkg-config to find CURL
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)

# Add the example executable
add_executable(basic_auth basic_auth.cpp)

# Include directories
target_include_directories(basic_auth PRIVATE 
    ${CMAKE_SOURCE_DIR}/../../include
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

# Link directories
target_link_directories(basic_auth PRIVATE
    ${CURL_LIBRARY_DIRS}
    ${OPENSSL_LIBRARY_DIRS}
)

# Link libraries
target_link_libraries(basic_auth PRIVATE 
    ${CMAKE_SOURCE_DIR}/../../build/libmpesa-cpp.a
    ${CURL_LIBRARIES}
    ssl
    crypto
)