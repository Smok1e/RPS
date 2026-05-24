#pragma once

#include <source_location>
#include <exception>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>

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

	int    check(int    ret,  std::source_location loc = std::source_location::current()) const;
	SOCKET check(SOCKET sock, std::source_location loc = std::source_location::current()) const;

};

//========================================