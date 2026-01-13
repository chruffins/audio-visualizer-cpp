cmake_minimum_required(VERSION 3.10)
project(audiovis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure a Debug build type by default for single-config generators and
# add GDB-friendly debug info (-ggdb) to the debug flags for GCC/Clang.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O3")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O3")
endif()

# Glob all source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(audiovis ${SOURCES})

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

# Link Allegro 5 libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO5 REQUIRED allegro-5 allegro_main-5 allegro_image-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5 allegro_audio-5 allegro_acodec-5 allegro_dialog-5)
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