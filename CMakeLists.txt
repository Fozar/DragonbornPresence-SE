cmake_minimum_required(VERSION 3.28)
project(DragonbornPresence VERSION 2.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# We build a DLL plugin, not an installable package — skip all install() rules
# (prevents CommonLibSSE's install(EXPORT) from failing on FetchContent targets)
set(CMAKE_SKIP_INSTALL_RULES TRUE)

# fmt — OVERRIDE_FIND_PACKAGE intercepts CommonLibSSE's find_package(fmt)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(fmt)

# spdlog — built against the fmt above
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# rapidcsv — single header only; set path before CommonLibSSE's find_path runs
FetchContent_Declare(
    rapidcsv
    GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
    GIT_TAG v8.83
    GIT_SHALLOW TRUE
    DOWNLOAD_ONLY YES
)
FetchContent_MakeAvailable(rapidcsv)
set(RAPIDCSV_INCLUDE_DIRS "${rapidcsv_SOURCE_DIR}/src" CACHE PATH "" FORCE)

# nlohmann/json — header-only JSON parser for locale file
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# CommonLibSSE-NG — find_package(fmt/spdlog) intercepted above, tests disabled
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG v3.7.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CommonLibSSE)
# add_commonlibsse_plugin is defined here, not exported by CommonLibSSE's install config
include("${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake")

# Discord Game SDK (pre-built zip, downloaded directly to avoid ExternalProject URL issues)
set(_discord_sdk_dir "${CMAKE_CURRENT_BINARY_DIR}/_discord_game_sdk")
if(NOT EXISTS "${_discord_sdk_dir}/cpp/discord.h")
    message(STATUS "Downloading Discord Game SDK 3.2.1...")
    set(_discord_sdk_zip "${CMAKE_CURRENT_BINARY_DIR}/_discord_game_sdk.zip")
    file(DOWNLOAD
        "https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip"
        "${_discord_sdk_zip}"
        SHOW_PROGRESS
        STATUS _discord_dl_status
    )
    list(GET _discord_dl_status 0 _discord_dl_err)
    if(_discord_dl_err)
        message(FATAL_ERROR
            "Failed to download Discord Game SDK: ${_discord_dl_status}\n"
            "Download discord_game_sdk.zip from https://discord.com/developers/docs/game-sdk/sdk-starter-guide "
            "and extract it to: ${_discord_sdk_dir}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${_discord_sdk_zip}" DESTINATION "${_discord_sdk_dir}")
    file(REMOVE "${_discord_sdk_zip}")
endif()

file(GLOB DISCORD_SDK_CPP "${_discord_sdk_dir}/cpp/*.cpp")
add_library(discord_cpp STATIC ${DISCORD_SDK_CPP})
target_include_directories(discord_cpp PUBLIC "${_discord_sdk_dir}/cpp")
target_link_libraries(discord_cpp PUBLIC
    "${_discord_sdk_dir}/lib/x86_64/discord_game_sdk.dll.lib"
)
set_target_properties(discord_cpp PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

add_commonlibsse_plugin(DragonbornPresence
    SOURCES
        main.cpp
        DragonbornPresence.cpp
        AdditionalFunctions.cpp
        discord_loader.cpp
    NAME "DragonbornPresence"
    VERSION "${PROJECT_VERSION}"
    USE_ADDRESS_LIBRARY
)

target_include_directories(DragonbornPresence PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${rapidcsv_SOURCE_DIR}/src"
)

target_precompile_headers(DragonbornPresence PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/pch.h"
)

target_link_libraries(DragonbornPresence PRIVATE discord_cpp delayimp nlohmann_json::nlohmann_json)
target_link_options(DragonbornPresence PRIVATE /DELAYLOAD:discord_game_sdk.dll)

set(_pkg_plugins "${CMAKE_CURRENT_BINARY_DIR}/_package/SKSE/Plugins")
file(MAKE_DIRECTORY "${_pkg_plugins}")

# Copy FOMOD installer, locale files, and default config into the package (configure time)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/fomod"   DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/_package")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/locales" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/_package")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/DragonbornPresenceConfig.json" DESTINATION "${_pkg_plugins}")

add_custom_command(TARGET DragonbornPresence POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_discord_sdk_dir}/lib/x86_64/discord_game_sdk.dll"
        "$<TARGET_FILE_DIR:DragonbornPresence>/discord_game_sdk.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:DragonbornPresence>"
        "${_pkg_plugins}/DragonbornPresence.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_discord_sdk_dir}/lib/x86_64/discord_game_sdk.dll"
        "${_pkg_plugins}/discord_game_sdk.dll"
    COMMAND ${CMAKE_COMMAND} -E tar cf
        "${CMAKE_CURRENT_BINARY_DIR}/DragonbornPresence.zip" --format=zip
        -- "SKSE/Plugins/DragonbornPresence.dll"
           "SKSE/Plugins/discord_game_sdk.dll"
           "SKSE/Plugins/DragonbornPresenceConfig.json"
           "fomod"
           "locales"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_package"
    COMMENT "Packaging DragonbornPresence.zip"
)
