cmake_minimum_required(VERSION 3.28)

# Standalone protocol / loopback tests for mod/src/net. Not the ASI build.
# Win32, static CRT, ENet v1.3.18, spdlog v1.17.0. Output: dh2net_test.exe.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
project(dh2net_test LANGUAGES C CXX)

if(NOT MSVC)
  message(FATAL_ERROR "dh2net_test requires MSVC. Run mod/net-test/build.ps1 or a vcvars32 shell.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "dh2net_test must be built as Win32/x86. Use vcvars32.bat.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_definitions(
  WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
  _WIN32_WINNT=0x0601)
add_compile_options(/utf-8 /EHsc /Zi)
add_link_options(/DEBUG /OPT:REF /OPT:ICF)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE)

FetchContent_Declare(enet
  GIT_REPOSITORY https://github.com/lsalzman/enet.git
  GIT_TAG v1.3.18
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR _no_cmake_project_)

FetchContent_MakeAvailable(spdlog enet)

add_library(enet_lib STATIC
  ${enet_SOURCE_DIR}/callbacks.c ${enet_SOURCE_DIR}/compress.c ${enet_SOURCE_DIR}/host.c
  ${enet_SOURCE_DIR}/list.c ${enet_SOURCE_DIR}/packet.c ${enet_SOURCE_DIR}/peer.c
  ${enet_SOURCE_DIR}/protocol.c ${enet_SOURCE_DIR}/win32.c)
target_include_directories(enet_lib PUBLIC ${enet_SOURCE_DIR}/include)
target_compile_definitions(enet_lib PRIVATE HAS_GETADDRINFO=1 HAS_GETNAMEINFO=1 HAS_INET_PTON=1 HAS_INET_NTOP=1)
target_link_libraries(enet_lib PUBLIC ws2_32 winmm)
target_compile_options(enet_lib PRIVATE /w)

file(GLOB NET_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/*.cpp)

add_library(dh2net STATIC ${NET_SRC})
target_include_directories(dh2net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_features(dh2net PUBLIC cxx_std_20)
target_compile_options(dh2net PRIVATE /W4 /permissive- /wd4100)
target_link_libraries(dh2net PUBLIC enet_lib spdlog::spdlog)

add_executable(dh2net_test main.cpp)
target_compile_features(dh2net_test PRIVATE cxx_std_20)
target_compile_options(dh2net_test PRIVATE /W4 /permissive- /wd4100)
target_link_libraries(dh2net_test PRIVATE dh2net)

# UPnP router logic, worker thread and the wrong-password guard (offline), plus --live for the real router.
add_executable(dh2upnp_test upnp_test.cpp)
target_compile_features(dh2upnp_test PRIVATE cxx_std_20)
target_compile_options(dh2upnp_test PRIVATE /W4 /permissive- /wd4100)
target_link_libraries(dh2upnp_test PRIVATE dh2net)
