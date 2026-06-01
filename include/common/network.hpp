#pragma once

#include <source_location>
#include <exception>
#include <string>
#include <cstring>

#ifdef RPS_WINDOWS

#include <WinSock2.h>
#include <WS2tcpip.h>

#else

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>

#endif // RPS_WINDOWS

//========================================

#ifdef NETWORK_VERBOSE

#define DEBUG_LOG(...) std::println(__VA_ARGS__)
#define DEBUG_DUMP_HEX(...) DumpHex(__VA_ARGS__)

#else

#define DEBUG_LOG(...)
#define DEBUG_DUMP_HEX(...)

#endif // NETWORK_VERBOSE

//========================================

class NetworkError
{
public:
	NetworkError(std::source_location loc = std::source_location::current());

	const std::source_location& getLocation() const;
	const std::string& getMessage() const;

private:
	std::source_location m_loc;
	std::string m_message;

};

//========================================

class Network
{
public:
#ifdef RPS_WINDOWS
	using socket_t = SOCKET;
	using ret_t = int;
	using socklen_t = int;
	
	static const socket_t InvalidSocket = INVALID_SOCKET;
#else
	using socket_t = int;
	using ret_t = ssize_t;
	using socklen_t = ::socklen_t;
	
	static const socket_t InvalidSocket = -1;
#endif // RPS_WINDOWS
	
	Network();
	~Network();

	static ret_t Check(
		ret_t ret,
		std::source_location loc = std::source_location::current()
	);

	static socket_t Check(
		socket_t sock,
		std::source_location loc = std::source_location::current()
	);
	
	static void Close(
		socket_t sock,
		std::source_location loc = std::source_location::current()
	);

};

//========================================