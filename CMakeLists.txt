# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(mpesa-cpp VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Ensure no compiler-specific extensions are used

# Add dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Library target
add_library(${PROJECT_NAME} STATIC
    # Source files will be added here
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Add examples
add_subdirectory(examples)