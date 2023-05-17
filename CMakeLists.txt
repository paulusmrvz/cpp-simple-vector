cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(cpp-simple-vector LANGUAGES C CXX)

add_executable(${PROJECT_NAME} simple-vector/simple_vector.h simple-vector/main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)