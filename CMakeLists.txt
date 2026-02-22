cmake_minimum_required(VERSION 3.14)
project(WinSSTV LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Multimedia REQUIRED)

# Include directories
include_directories(
    src/core
    src/core/sstv
    src/core/dsp
    src/core/utils
    src/core/sound
    src/stubs
)

# Source files
file(GLOB_RECURSE SOURCES
    "src/core/*.cpp"
    "src/stubs/*.cpp"
    "src/stubs/*.h"
    "src/core/*.h"
)

# Exclude original audio implementations from mixed sources if any were accidentally copied or globbed
# (We only copied soundbase, but just in case)

add_executable(WinSSTV ${SOURCES})

target_link_libraries(WinSSTV PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia)
