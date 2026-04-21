cmake_minimum_required(VERSION 3.10)
project(audiovis)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_CCACHE "Use ccache to accelerate rebuilds" ON)
option(ENABLE_UNITY_BUILD "Enable CMake unity/jumbo builds for faster full builds" OFF)
option(ENABLE_PCH "Enable precompiled headers for C++ sources" ON)

if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        if(NOT CMAKE_C_COMPILER_LAUNCHER)
            set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        endif()
        if(NOT CMAKE_CXX_COMPILER_LAUNCHER)
            set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        endif()
    endif()
endif()

# Ensure a Debug build type by default for single-config generators and
# add GDB-friendly debug info (-ggdb) to the debug flags for GCC/Clang.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

if(NOT MSVC AND NOT WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -Os -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -Os -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    
    # Release build: aggressive size optimization
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -s")
endif()

# Glob all source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

add_executable(audiovis ${SOURCES})

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    if(ENABLE_UNITY_BUILD)
        set_target_properties(audiovis PROPERTIES UNITY_BUILD ON)
    endif()

    if(ENABLE_PCH)
        target_precompile_headers(audiovis PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/pch.hpp>"
        )
    endif()
endif()

# Build bundled libscrob and make its target available for linking.
add_subdirectory(external/libscrob)

# Define some handy Social SDK variables
set(DISCORD_SDK_ROOT "${CMAKE_SOURCE_DIR}/external/discord-sdk")
set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

# Include for Social SDK headers
target_include_directories(audiovis PRIVATE ${DISCORD_SDK_INCLUDE_DIR})

# Include directories for your project
target_include_directories(audiovis PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Platform-specific Social SDK library paths
if(WIN32)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/discord_partner_sdk.lib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_DIR}/discord_partner_sdk.dll")
elseif(APPLE)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
else() # Linux
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
endif()

# Link the Social SDK library
target_link_libraries(audiovis PRIVATE ${DISCORD_LIB_PATH})

# Link libscrob (external/libscrob)
target_link_libraries(audiovis PRIVATE scrob)

# Link Allegro 5 libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO5 REQUIRED allegro-5 allegro_main-5 allegro_image-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5 allegro_audio-5 allegro_acodec-5 allegro_dialog-5 allegro_memfile-5)
target_include_directories(audiovis PRIVATE ${ALLEGRO5_INCLUDE_DIRS})
target_link_libraries(audiovis PRIVATE ${ALLEGRO5_LIBRARIES})
target_compile_options(audiovis PRIVATE ${ALLEGRO5_CFLAGS_OTHER})

# Link SQLite3 library
find_package(SQLite3 REQUIRED)
target_include_directories(audiovis PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(audiovis PRIVATE ${SQLite3_LIBRARIES})
target_compile_options(audiovis PRIVATE ${SQLite3_CFLAGS_OTHER})

# Link TagLib library
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
target_include_directories(audiovis PRIVATE ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(audiovis PRIVATE ${TAGLIB_LIBRARIES})
target_compile_options(audiovis PRIVATE ${TAGLIB_CFLAGS_OTHER})

# Set the runtime search path (RPATH) for Linux and macOS
if(UNIX)
    # Use RPATH when building
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # Set the RPATH to use the lib directory relative to the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    endif()
endif()

# Copy Social SDK shared library to output directory, so it's available at runtime.
add_custom_command(TARGET audiovis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DISCORD_SHARED_LIB}"
    $<TARGET_FILE_DIR:audiovis>;
)

install(TARGETS audiovis
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY assets/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/audiovis/assets
)

if(UNIX AND NOT APPLE)
    install(FILES "${DISCORD_SHARED_LIB}"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()