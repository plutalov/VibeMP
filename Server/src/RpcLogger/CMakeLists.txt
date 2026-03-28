cmake_minimum_required(VERSION 3.14)
project(RpcLogger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Path to omp-src submodule from this directory
set(OMP_ROOT ${CMAKE_SOURCE_DIR}/../../../omp-src)

add_library(RpcLogger SHARED dllmain.cpp)

target_include_directories(RpcLogger PRIVATE
    ${OMP_ROOT}/SDK/include
    ${OMP_ROOT}/SDK/lib/glm
    ${OMP_ROOT}/SDK/lib/robin-hood-hashing/src/include
    ${OMP_ROOT}/SDK/lib/span-lite/include
    ${OMP_ROOT}/SDK/lib/string-view-lite/include
    ${OMP_ROOT}/Shared/Network
)
