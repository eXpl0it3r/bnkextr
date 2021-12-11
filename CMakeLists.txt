cmake_minimum_required(VERSION 3.15)

project(bnkextr LANGUAGES CXX)

add_executable(${PROJECT_NAME} "bnkextr.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# Static Runtime
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND NOT APPLE)
	target_link_options(${PROJECT_NAME} PRIVATE -static)
endif()

# Install executable
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION .)