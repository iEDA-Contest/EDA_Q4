cmake_minimum_required(VERSION 3.16)
set(THIS EDA_CHALLENGE_Q4)
project (${THIS})
include_directories(includes)
aux_source_directory(./src CPPSRC)
aux_source_directory(./legalization CPPSRC)

add_executable(${THIS}.out ${CPPSRC})

if (debug STREQUAL "1")
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -ggdb3 -O0")
else()
SET(CMAKE_BUILD_TYPE "RELEASE")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wall -O2")
endif()
