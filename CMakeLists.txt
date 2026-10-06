cmake_minimum_required(VERSION 3.21)

project(
    GoWR-LuaLoader
    VERSION 1.0.1
    DESCRIPTION "Lua script loader for God of War Ragnarok, with crash logging"
    HOMEPAGE_URL "https://github.com/PhiloSocio/GoWR-LuaLoader"
    LANGUAGES C CXX
)
set(ADD_PLUGIN_AUTHOR "AnArchos" CACHE STRING "Plugin author")
set(ADD_PLUGIN_EMAIL  "patreon.com/AnArchos" CACHE STRING "Plugin email")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(
    QuickDllProxy
    GIT_REPOSITORY https://github.com/Nukem9/QuickDllProxy.git
    GIT_TAG master
)
FetchContent_MakeAvailable(QuickDllProxy)

find_package(Lua REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h" "src/*.hpp" "src/*.inc")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

configure_file(
    resources/version.rc.in
    ${CMAKE_BINARY_DIR}/version.rc
    @ONLY
)
add_library(version SHARED
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_BINARY_DIR}/version.rc
)

target_include_directories(version PRIVATE
    src
    ${LUA_INCLUDE_DIRS}
)

target_link_libraries(version PRIVATE
    ${LUA_LIBRARIES}
    dbghelp
    spdlog::spdlog
    toml11::toml11
)

target_compile_definitions(version PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    LOADER_NAME="${PROJECT_NAME}"
    LOADER_VERSION="${PROJECT_VERSION}"
    LOADER_AUTHOR="${ADD_PLUGIN_AUTHOR}"
    LOADER_EMAIL="${ADD_PLUGIN_EMAIL}"
)

set_target_properties(version PROPERTIES
    PREFIX ""
    OUTPUT_NAME "winmm"
)

if(MSVC)
    target_link_options(version PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF>
    )
endif()
