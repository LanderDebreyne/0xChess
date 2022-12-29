cmake_minimum_required(VERSION 3.16)

project(0xChess CXX)

# Set the default build the to Debug if it's not set on the command line.
# This is not done for multi configuration generator like Visual Studio
# (detected thought CMAKE_CONFIGURATION_TYPES).
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)

    # Set the possible values of build type for cmake-gui/ccmake
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else ()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif ()

# Add the executable
add_executable(
    0xChess
    main.cpp
)