cmake_minimum_required(VERSION 3.1)

project(heliage-advance)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(HA_FRONTEND "SDL2" CACHE STRING "The frontend heliage-advance will run on")
set_property(CACHE HA_FRONTEND PROPERTY STRINGS SDL2 Null)
if (${HA_FRONTEND} MATCHES "SDL2")
    find_package(SDL2 REQUIRED)
    add_compile_definitions("HA_FRONTEND_SDL")
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-gnu-zero-variadic-macro-arguments
    )
endif()

set(SOURCES
    src/arm7/arm7.cpp
    src/arm7/disassembler.cpp
    src/arm7/arm/arm.cpp
    src/arm7/arm/branch.cpp
    src/arm7/arm/data_processing.cpp
    src/arm7/thumb/branch.cpp
    src/arm7/thumb/thumb.cpp
    src/bios.cpp
    src/bus.cpp
    src/cartridge.cpp
    src/gba.cpp
    src/main.cpp
    src/ppu.cpp
    src/timer.cpp
)

set(HEADERS
    src/common/bits.h
    src/common/defines.h
    src/common/logging.h
    src/common/types.h
    src/arm7/arm7.h
    src/bios.h
    src/bus.h
    src/cartridge.h
    src/gba.h
    src/keypad.h
    src/ppu.h
    src/timer.h
)

if (${HA_FRONTEND} MATCHES "SDL2")
    set(SOURCES ${SOURCES} "src/frontend/sdl.cpp")
    set(HEADERS ${HEADERS} "src/frontend/sdl.h")
else()
    set(SOURCES ${SOURCES} "src/frontend/null.cpp")
    set(HEADERS ${HEADERS} "src/frontend/null.h")
endif()

add_subdirectory(dependencies/range-v3)
add_subdirectory(dependencies/fmt)

add_executable(heliage-advance ${SOURCES} ${HEADERS})
target_include_directories(heliage-advance PRIVATE
    src
    dependencies
    dependencies/range-v3/include
    dependencies/fmt
)
target_link_libraries(heliage-advance fmt)
if (${HA_FRONTEND} MATCHES "SDL2")
    target_link_libraries(heliage-advance SDL2)
endif()
