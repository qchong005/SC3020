cmake_minimum_required(VERSION 3.14)
project(DBMS_Task1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_executable(dbms
  src/main.cpp
  src/disk.cpp
  src/buffer.cpp
  src/loader.cpp
)

target_include_directories(dbms PRIVATE include)

# Put the executable into a bin/ folder inside the build directory
set_target_properties(dbms PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
