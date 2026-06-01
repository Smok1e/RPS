cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(RPS CXX)

option(
	NETWORK_VERBOSE 
	"Enable additional protocol logging"
	OFF
)

#======================================== External packages

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

# CMake < 4.3
if (NOT TARGET SQLite3::SQLite3)
	add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif ()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ArgParser/CMakeLists.txt")
	message(FATAL_ERROR "external/ArgParser does not contains CMakeLists.txt; make sure to initialize git submodules")
endif ()

add_subdirectory(external/ArgParser)

#======================================== Common sources

add_library(
	common
	STATIC
	"src/common/network.cpp"
	"src/common/packet.cpp"
	"src/common/peer.cpp"
	"src/common/score_provider.cpp"
	"src/common/move.cpp"
)

target_include_directories(
	common
	PUBLIC
	"include/"
)

target_link_libraries(
	common
	PUBLIC
	ArgParser
	OpenSSL::Crypto
	SQLite3::SQLite3
)

target_compile_definitions(
	common
	PUBLIC
	$<$<BOOL:${NETWORK_VERBOSE}>:NETWORK_VERBOSE>
)

if (WIN32)
	target_compile_definitions(
		common
		PUBLIC
		RPS_WINDOWS
	)
	
	target_link_libraries(
		common
		PUBLIC
		wsock32
		ws2_32
	)
endif ()

#======================================== Server

add_executable(
	server
	"src/server/main.cpp"
	"src/server/server.cpp"
	"src/server/server_peer.cpp"
)

target_link_libraries(server PRIVATE common)

#======================================== Client

add_executable(
	client
	"src/client/main.cpp"
	"src/client/client_peer.cpp"
)

target_link_libraries(client PRIVATE common)

#========================================

add_executable(
	test
	"src/test.cpp"
	"src/common/score_provider"
)

target_link_libraries(test PRIVATE common)