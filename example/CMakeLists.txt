# ------------------------------------------
# User config between these lines

# Set project name
set(EXAMPLE "repl")

# ------------------------------------------

project(${EXAMPLE} CXX)

# Define game source files
file(GLOB_RECURSE EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${EXAMPLE} ${EXAMPLE_SOURCES})
target_include_directories(${EXAMPLE} PRIVATE
	"src")
target_link_libraries(${EXAMPLE} ${PROJECT})

# ------------------------------------------

# Add 'make run' target
add_custom_target(run
	COMMAND ${EXAMPLE} -c
	DEPENDS ${PROJECT}
	WORKING_DIRECTORY "..")
