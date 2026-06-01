#pragma once

#include <source_location>
#include <exception>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>

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
	Network();
	~Network();

	static int Check(
		int ret,  
		std::source_location loc = std::source_location::current()
	);

	static SOCKET Check(
		SOCKET sock, 
		std::source_location loc = std::source_location::current()
	);

};

//========================================