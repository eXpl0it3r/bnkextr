cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)

project(bnkextr)

add_executable(bnkextr "bnkextr.cpp")

# Static Runtime
if(WIN32 AND MSVC AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15.0")
	set_property(TARGET bnkextr PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
