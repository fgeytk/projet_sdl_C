cmake_minimum_required(VERSION 3.24)

project(game_of_life_sdl LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output")

option(GOL_USE_SYSTEM_SDL3 "Use a system-installed SDL3 before downloading it" ON)

if(GOL_USE_SYSTEM_SDL3)
    find_package(SDL3 CONFIG QUIET COMPONENTS SDL3)
endif()

if(NOT TARGET SDL3::SDL3)
    include(FetchContent)

    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.0.zip
    )
    FetchContent_MakeAvailable(SDL3)
endif()

add_library(game_of_life_core
    src/config.c
    src/io.c
    src/simulation.c
    src/world.c
)

target_include_directories(game_of_life_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(game_of_life
    src/app.c
    src/main.c
)

target_link_libraries(game_of_life PRIVATE game_of_life_core)

target_link_libraries(game_of_life PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(game_of_life PRIVATE /W4)
else()
    target_compile_options(game_of_life PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    add_custom_command(TARGET game_of_life POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:game_of_life>
            $<TARGET_FILE_DIR:game_of_life>
        COMMAND_EXPAND_LISTS
    )
endif()