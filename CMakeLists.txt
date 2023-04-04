# ------------------------------------------
# User config between these lines

# Set project name
set(PROJECT "blaze")

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(BLAZE_STANDALONE TRUE)
endif()

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BLAZE_BUILD_EXAMPLES "Build the Blaze example programs" ${BLAZE_STANDALONE})

# ------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(${PROJECT} CXX C)

# ------------------------------------------
# Setup C++ compiler

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags used for all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
# -Wall      = All warnings about contructions that are easily avoidable
# -Wextra    = Extra warning flags not covered by -Wall
# -Wpedantic = Warnings for compiler extensions not part of the standard

# Set default build type if not specified
set(DEFAULT_BUILD_TYPE Release)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git)
	set(DEFAULT_BUILD_TYPE Debug)
endif()
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE})
endif()

# Set build type specific compiler flags
message("--- ${CMAKE_BUILD_TYPE} ---")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g -pg")
	# -Og = Optimizations that do not interfere with debugging
	# -g  = Produce debugging information in OS's native format
	# -pg = Generate profile information for analysis with gprof
	# $ gprof <PROJECT> gmon.out > profile-data.txt
elseif(${CMAKE_BUILD_TYPE} STREQUAL Release)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
	# -O3 = Optimizations that increase compilation time and performance
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------
# Library

add_subdirectory("vendor/ruc")

# ------------------------------------------
# Application target

# Define source files
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp")

add_executable(${PROJECT} ${PROJECT_SOURCES})
target_include_directories(${PROJECT} PRIVATE
	"src")
target_link_libraries(${PROJECT} readline ruc)

# ------------------------------------------
# Execute target

add_custom_target(run
    COMMAND ${PROJECT} -c
	DEPENDS ${PROJECT})

# ------------------------------------------
# Test targets

add_custom_target(test0
    COMMAND env STEP=step0_repl MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step0_repl.mal -- ./${PROJECT})
add_dependencies(test0 ${PROJECT})

add_custom_target(test1
    COMMAND env STEP=step1_read_print MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step1_read_print.mal -- ./${PROJECT})
add_dependencies(test1 ${PROJECT})

add_custom_target(test2
    COMMAND env STEP=step_eval MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step2_eval.mal -- ./${PROJECT})
add_dependencies(test2 ${PROJECT})

add_custom_target(test3
    COMMAND env STEP=step_env MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step3_env.mal -- ./${PROJECT})
add_dependencies(test3 ${PROJECT})

add_custom_target(test4
    COMMAND env STEP=step_env MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step4_if_fn_do.mal -- ./${PROJECT})
add_dependencies(test4 ${PROJECT})

add_custom_target(test5
    COMMAND env STEP=step_env MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step5_tco.mal -- ./${PROJECT})
add_dependencies(test5 ${PROJECT})

add_custom_target(test6
    COMMAND env STEP=step_env MAL_IMPL=js ../vendor/mal/runtest.py --deferrable --optional ../vendor/mal/tests/step6_file.mal -- ./${PROJECT})
add_dependencies(test6 ${PROJECT})
