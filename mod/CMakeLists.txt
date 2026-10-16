cmake_minimum_required(VERSION 3.28)

# Co-op mod ASI plugin. Win32 (x86) only, static CRT. Build via mod/build.ps1.
project(dh2coop LANGUAGES C CXX)

if(NOT MSVC)
  message(FATAL_ERROR "dh2coop requires MSVC (v143). Run mod/build.ps1 or a vcvars32 shell.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "dh2coop must be built as Win32/x86 (the game is 32-bit). Use vcvars32.bat.")
endif()

# ---- global settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)              # safetyhook v0.7.0 requires cxx_std_23 (<expected>); that
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # PUBLIC feature raises our target to /std:c++latest.
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT (/MT) for every target, including FetchContent dependencies.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# Dependencies that declare an ancient cmake_minimum_required still configure on CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_definitions(
  WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
  _WIN32_WINNT=0x0601)
add_compile_options(/utf-8 /Zi)
add_link_options(/DEBUG /OPT:REF /OPT:ICF)

# ---- dependencies (pinned tags) --------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# safetyhook v0.7.0 fetches Zydis v4.1.0 itself (SAFETYHOOK_FETCH_ZYDIS=ON).
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_DOXYGEN  OFF CACHE BOOL "" FORCE)
set(ZYDIS_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_BUILD_TEST     OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_Declare(safetyhook
  GIT_REPOSITORY https://github.com/cursey/safetyhook.git
  GIT_TAG        v0.7.0
  GIT_SHALLOW    TRUE)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
  GIT_SHALLOW    TRUE)

# Dear ImGui and ENet have no (or unsuitable) CMake projects: fetch sources only, define targets below.
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.93.0
  GIT_SHALLOW    TRUE)

FetchContent_Declare(enet
  GIT_REPOSITORY https://github.com/lsalzman/enet.git
  GIT_TAG        v1.3.18
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  _no_cmake_project_)   # skip enet's own CMakeLists; we build it as a static lib

FetchContent_MakeAvailable(safetyhook spdlog imgui enet)

# ---- imgui (added, not wired yet) --------------------------------------------
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_features(imgui_lib PUBLIC cxx_std_17)

# ---- enet ------------------------------------------------------------------
add_library(enet_lib STATIC
  ${enet_SOURCE_DIR}/callbacks.c ${enet_SOURCE_DIR}/compress.c ${enet_SOURCE_DIR}/host.c
  ${enet_SOURCE_DIR}/list.c ${enet_SOURCE_DIR}/packet.c ${enet_SOURCE_DIR}/peer.c
  ${enet_SOURCE_DIR}/protocol.c ${enet_SOURCE_DIR}/win32.c)
target_include_directories(enet_lib PUBLIC ${enet_SOURCE_DIR}/include)
target_compile_definitions(enet_lib PRIVATE HAS_GETADDRINFO=1 HAS_GETNAMEINFO=1 HAS_INET_PTON=1 HAS_INET_NTOP=1)
target_link_libraries(enet_lib PUBLIC ws2_32 winmm)
target_compile_options(enet_lib PRIVATE /w)  # third-party C code: silence warnings

# ---- the ASI ---------------------------------------------------------------
add_library(dh2coop SHARED
  src/dllmain.cpp
  src/core/config.cpp
  src/core/exports.cpp
  src/core/log.cpp
  src/core/safe_mem.cpp
  src/probe/probe.cpp)
target_include_directories(dh2coop PRIVATE src)
target_compile_features(dh2coop PRIVATE cxx_std_20)
target_compile_options(dh2coop PRIVATE /W4 /permissive- /wd4100)
target_link_libraries(dh2coop PRIVATE safetyhook::safetyhook spdlog::spdlog enet_lib imgui_lib)
set_target_properties(dh2coop PROPERTIES PREFIX "" SUFFIX ".asi" OUTPUT_NAME "dh2coop")
