cmake_minimum_required(VERSION 3.15)
project(fmu_storage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(fmu_storage
	src/api.cpp
)

target_include_directories(fmu_storage PUBLIC include)

add_executable(fmu_example examples/main.cpp)
target_link_libraries(fmu_example PRIVATE fmu_storage)
