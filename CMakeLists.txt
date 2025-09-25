cmake_minimum_required(VERSION 3.10)

project(DatabaseSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Find all source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(main ${SOURCES})

# Set output directory to project root
set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)