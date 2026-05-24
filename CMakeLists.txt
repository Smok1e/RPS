cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(RPS)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ArgParser/CMakeLists.txt")
	message(FATAL_ERROR "external/ArgParser does not contains CMakeLists.txt")
endif ()

add_subdirectory(external/ArgParser)

add_executable(
	server
	src/network.cpp
	src/packet.cpp
	src/server/main.cpp
	src/server/server.cpp
	src/server/client_handler.cpp
)

target_include_directories(
	server 
	PRIVATE 
	include/
)

target_link_libraries(
	server
	PRIVATE
	ArgParser
	wsock32 
	ws2_32
)
