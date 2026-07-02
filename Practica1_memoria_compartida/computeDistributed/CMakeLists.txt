cmake_minimum_required(VERSION 3.16)
project(Practica1b_Distribuida LANGUAGES C CXX)

set(CMAKE_C_COMPILER mpicc)
set(CMAKE_CXX_COMPILER mpicxx)

find_package(MPI REQUIRED)

add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic)

add_executable(computeDistributed
    main.cpp
)

target_include_directories(computeDistributed PRIVATE
    ${CMAKE_SOURCE_DIR}/libDomain/include
)

target_link_libraries(computeDistributed PRIVATE 
    MPI::MPI_CXX
    libDomain
)

