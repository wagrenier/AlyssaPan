cmake_minimum_required(VERSION 3.28)
project(AlyssaPan LANGUAGES C CXX)

# PC port of Clock Tower 3 (USA, SLUS_206.33), reconstructed function by function. The Pan layer is
# YamiPan's (Forbidden Siren 2), carried over and trimmed to what is not FS2's own.
# src/alyssapan is the Pan layer (modern C++), src/sdk SCE's API on top of it, and everything else
# the game, kept authentic (.claude/skills/alyssapan-port/references/cpp-port.md).

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AlyssaPan)
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(WIN32 AND NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# The prebuilt FFmpeg and shadercross trees are not committed: a sibling checkout's are reused.
set(ALYSSAPAN_SIBLING_EXTERN "${CMAKE_CURRENT_SOURCE_DIR}/../MioPan/extern"
    CACHE PATH "extern/ of a checkout holding the prebuilt FFmpeg and shadercross")

include(sdl3)
include(imgui)
include(shaders)
include(ffmpeg)

# ---- the game layer, with SCE's API under it -----------------------------------------------------
# CT3's game code is C (GCC 2.9x): src/game/ for the game, src/ga/ for its 3D library (ga*), src/iop/ for the
# IOP code that runs on the host's IOP; directories are added as TUs are read (architecture.md "Modules").
file(GLOB_RECURSE ALYSSAPAN_GAME_SOURCES CONFIGURE_DEPENDS
    src/game/*.c src/game/*.cpp
    src/ga/*.c src/ga/*.cpp
    src/iop/*.c src/iop/*.cpp
    src/lib/*.c src/lib/*.cpp
    src/sdk/*.c src/sdk/*.cpp)

# The game's own .c files keep the names its strings give them (movie.c, sound.c, xcmd_tn.c); like every
# TU in RosePan and YamiPan they build as C++, since the SDK's declarations are C++ here.
set_source_files_properties(${ALYSSAPAN_GAME_SOURCES} PROPERTIES LANGUAGE CXX)

# The process entry belongs to the host (src/main.cpp); the game's main stays as the ROM has it, and its
# file (game/main.c, written 2026-11-09) is built with main=gameMain.
set_source_files_properties(src/game/main.c PROPERTIES COMPILE_DEFINITIONS main=gameMain)

add_library(alyssapan_game OBJECT ${ALYSSAPAN_GAME_SOURCES})
target_include_directories(alyssapan_game PUBLIC src src/sdk)
target_link_libraries(alyssapan_game PUBLIC SDL3::SDL3)
if(MSVC)
    # The flags layoutcheck.py compiles the game with; /utf-8 reads the sources as GCC does, or the
    # Japanese literals change bytes. cl does not exploit aliasing, so GCC's flag below has no counterpart.
    target_compile_options(alyssapan_game PRIVATE /permissive /Zc:preprocessor /utf-8 /w)
else()
    # NOT OPTIONAL. Reconstructed code reads the game's file formats in place and walks packets through
    # u_int*; GCC exploits the aliasing violation from -O2 up (MioPan's release-only crashes).
    target_compile_options(alyssapan_game PRIVATE -fno-strict-aliasing -fpermissive -w)
endif()

# ---- the Pan layer -------------------------------------------------------------------------------
file(GLOB_RECURSE ALYSSAPAN_HOST_SOURCES CONFIGURE_DEPENDS src/alyssapan/*.cpp)

add_executable(AlyssaPan src/main.cpp ${ALYSSAPAN_HOST_SOURCES} $<TARGET_OBJECTS:alyssapan_game>)
add_dependencies(AlyssaPan compile_shaders)
target_include_directories(AlyssaPan PRIVATE src)
target_link_libraries(AlyssaPan PRIVATE SDL3::SDL3 imgui)
# The exe's icon (tools/make_icon.py draws it; resources/alyssapan.png is the window's, loaded at run time).
if(WIN32)
    enable_language(RC)
    target_sources(AlyssaPan PRIVATE extern/icon.rc)
    set_property(SOURCE extern/icon.rc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/extern/alyssapan.ico)
endif()
if(MSVC)
    target_compile_options(AlyssaPan PRIVATE /W3 /Zc:preprocessor /utf-8)
    target_compile_definitions(AlyssaPan PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(AlyssaPan PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wno-missing-field-initializers)
endif()

if(ALYSSAPAN_HAVE_FFMPEG)
    target_compile_definitions(AlyssaPan PRIVATE ALYSSAPAN_HAVE_FFMPEG=1)
    target_compile_definitions(alyssapan_game PRIVATE ALYSSAPAN_HAVE_FFMPEG=1)
    target_link_libraries(AlyssaPan PRIVATE ffmpeg::avcodec ffmpeg::avutil ffmpeg::swscale)
    add_custom_command(TARGET AlyssaPan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ALYSSAPAN_FFMPEG_ROOT}/bin $<TARGET_FILE_DIR:AlyssaPan>)
endif()

add_custom_command(TARGET AlyssaPan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:AlyssaPan>/resources)

# The build stamp in the log banner: scoped to logger.cpp so a new commit rebuilds only it.
find_package(Git QUIET)
set(ALYSSAPAN_BUILD_COMMIT unknown)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ALYSSAPAN_BUILD_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
set_source_files_properties(src/alyssapan/core/logger.cpp PROPERTIES
    COMPILE_DEFINITIONS "ALYSSAPAN_BUILD_COMMIT=\"${ALYSSAPAN_BUILD_COMMIT}\"")
