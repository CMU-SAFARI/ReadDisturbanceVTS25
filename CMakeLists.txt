cmake_minimum_required(VERSION 3.15...3.27)
project(DCT)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

#### Prompt the build type ####
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Flags used: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
###############################

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

######### Boost #########
find_package(Boost COMPONENTS thread program_options REQUIRED)
###################################



######### Nanobind #########
if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanobind)
###################################

add_library(DCT OBJECT
  src/api/board/xdma.cpp
  src/api/platform/platform.cpp
  src/api/platform/DDR4.cpp
  src/api/program/program.cpp
  src/api/program/instruction.cpp

  src/programs/init_row.cpp
  src/programs/init_rows.cpp
  src/programs/init_row_range.cpp

  src/programs/read_row.cpp
  src/programs/read_rows.cpp
  src/programs/read_row_range.cpp

  src/programs/act_row.cpp
  src/programs/act_rows.cpp
  src/programs/refresh_all_rows.cpp

  src/programs/singleside_hammer.cpp
  src/programs/doubleside_hammer.cpp

  src/programs/rowclone.cpp
  src/programs/tWR_test.cpp
  src/programs/frac.cpp
)


target_include_directories(DCT
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(DCT 
  PRIVATE Boost::thread
)

add_executable(test
  src/test.cpp
)

target_link_libraries(test 
  PRIVATE DCT
)

nanobind_add_module(pyDRAMBender
  python-bindings/pyDRAMBender.cpp
)

target_include_directories(pyDRAMBender
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(pyDRAMBender
  PRIVATE DCT
)