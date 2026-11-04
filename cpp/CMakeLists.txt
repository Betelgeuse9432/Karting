cmake_minimum_required(VERSION 3.21)
project(KartingNative VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KARTING_BUILD_GAME "Build the GPU-powered raylib game" ON)
option(KARTING_FETCH_RAYLIB "Fetch raylib 5.5 if not installed" ON)

add_library(karting_core STATIC src/core.cpp)
target_include_directories(karting_core PUBLIC "include")
target_compile_features(karting_core PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(karting_core PRIVATE /W4)
else()
  target_compile_options(karting_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(KARTING_BUILD_GAME)
  find_package(raylib 5.5 QUIET)
  if(NOT TARGET raylib)
    if(NOT KARTING_FETCH_RAYLIB)
      message(FATAL_ERROR "raylib 5.5 was not found; enable KARTING_FETCH_RAYLIB")
    endif()
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.5
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
  endif()

  add_executable(karting_cpp
    src/main.cpp
    src/game.cpp
    src/render.cpp
    src/effects.cpp
    src/ui.cpp)
  target_link_libraries(karting_cpp PRIVATE karting_core raylib)
  target_include_directories(karting_cpp PRIVATE "include")
  if(MSVC)
    target_compile_options(karting_cpp PRIVATE /W4)
  else()
    target_compile_options(karting_cpp PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(karting_cpp_tests tests/core_tests.cpp)
  target_link_libraries(karting_cpp_tests PRIVATE karting_core)
  add_test(NAME karting_core_tests COMMAND karting_cpp_tests)
endif()
