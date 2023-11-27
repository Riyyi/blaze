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
option(BLAZE_BUILD_TESTS "Build the Blaze test programs" ${BLAZE_STANDALONE})

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
# Dependencies

add_subdirectory("vendor/ruc")

# ------------------------------------------
# Library target

# Define source files
file(GLOB_RECURSE LIBRARY_SOURCES "src/*.cpp")

add_library(${PROJECT} ${LIBRARY_SOURCES})
target_include_directories(${PROJECT} PUBLIC
	"src")
target_link_libraries(${PROJECT} readline ruc)

# ------------------------------------------
# Std target

add_custom_target(${PROJECT}-lisp
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy-lisp.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(${PROJECT} ${PROJECT}-lisp)

# ------------------------------------------
# Example target

if (BLAZE_BUILD_EXAMPLES)
	add_subdirectory("example")
endif()

# ------------------------------------------
# Test targets

if (BLAZE_BUILD_TESTS)
	function(make_test_target target_name step_name)
		add_custom_target(${target_name}
			COMMAND ../vendor/mal/runtest.py --deferrable --optional ../tests/${step_name}.mal -- ./${PROJECT})
		add_dependencies(${target_name} ${PROJECT})
	endfunction()

	make_test_target("test0" "step0_repl")
	make_test_target("test1" "step1_read_print")
	make_test_target("test2" "step2_eval")
	make_test_target("test3" "step3_env")
	make_test_target("test4" "step4_if_fn_do")
	make_test_target("test5" "step5_tco")
	make_test_target("test6" "step6_file")
	make_test_target("test7" "step7_quote")
	make_test_target("test8" "step8_macros")
	make_test_target("test9" "step9_try")
	make_test_target("testA" "stepA_mal")

	function(make_host_test_target target_name step_name)
		add_custom_target(${target_name}
			COMMAND ../vendor/mal/runtest.py --deferrable --optional ../tests/${step_name}.mal -- ./${PROJECT} ../mal/${step_name}.mal)
		add_dependencies(${target_name} ${PROJECT})
	endfunction()

	make_host_test_target("host_test0" "step0_repl")
	make_host_test_target("host_test1" "step1_read_print")
	make_host_test_target("host_test2" "step2_eval")
	make_host_test_target("host_test3" "step3_env")
	make_host_test_target("host_test4" "step4_if_fn_do")
	# make_host_test_target("host_test5" "step5_tco") # disabled
	make_host_test_target("host_test6" "step6_file")
	make_host_test_target("host_test7" "step7_quote")
	make_host_test_target("host_test8" "step8_macros")
	make_host_test_target("host_test9" "step9_try")
	make_host_test_target("host_testA" "stepA_mal")

	add_custom_target(perf
		COMMAND ./${PROJECT} ../tests/perf1.mal
		COMMAND ./${PROJECT} ../tests/perf2.mal
		COMMAND ./${PROJECT} ../tests/perf3.mal)
	add_dependencies(perf ${PROJECT})
endif()
