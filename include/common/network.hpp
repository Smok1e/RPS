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

/// \brief Custom exception type for general network errors
///
/// Holds location information and error description retrieved
/// from the system. Thrown by Network::Check method when return
/// value indicates an error.
class NetworkError
{
public:
	/// \brief Constructor
	/// 
	/// \param loc Error location (current location by default)
	NetworkError(std::source_location loc = std::source_location::current());

	/// \brief Get error location
	///
	/// \return Error location
	const std::source_location& getLocation() const;

	/// \brief Get error message
	///
	/// \return Error message
	const std::string& getMessage() const;

private:
	std::source_location m_loc;
	std::string m_message;

};

//========================================

/// \brief OS-independent compatibility layer for socket communication
///
/// A collection of type abstractions and utility methods for socket
/// communication initialization and debugging. The program should allocate
/// exactly one instance of this class in order to initialize network API
/// properly on each supported system.
class Network
{
public:
#ifdef RPS_WINDOWS
	using socket_t = SOCKET; //!< Socket type
	using ret_t = int;       //!< Socket API functions return type
	using socklen_t = int;   //!< Socket length type
	
	static const socket_t InvalidSocket = INVALID_SOCKET; //!< Invalid socket value
#else
	using socket_t = int;		   //!< Socket type
	using ret_t = ssize_t;		   //!< Socket API functions return type
	using socklen_t = ::socklen_t; //!< Socket length type
	
	static const socket_t InvalidSocket = -1; //!< Invalid socket value
#endif // RPS_WINDOWS
	
	/// Default constructor
	Network();

	/// Destructor
	~Network();

	/// \brief Check socket API return value (general methods)
	///
	/// \param ret Expression value to check
	/// \return Same expression value
	/// 
	/// \throws NetworkError if the ret indicates an error
	/// 
	/// Should be used to wrap socket API calls (send, recv, bind, etc.). In case of
	/// error, throws an instance of NetworkError, containing error location and message.
	/// 
	/// Example:
	/// \code{.cpp}
	/// try
	/// {
	///		auto len = Network::Check(send(sock, buffer, sizeof(buffer), 0));
	/// }
	/// catch (const NetworkError& err)
	/// {
	///		std::println(
	///			"Network arror at line {}: {}", 
	///			err.getLocation().line(), 
	///			err.getMessage()
	///		);
	/// }
	/// \endcode
	/// 
	/// \see NetworkError
	static ret_t Check(
		ret_t ret,
		std::source_location loc = std::source_location::current()
	);

	/// \brief Check socket API return value
	///
	/// Should be used to wrap socket creation API calls (create, listen, etc.). In case of
	/// error, throws an instance of NetworkError, containing error location and message.
	/// 
	/// \param sock Socket instance to check
	/// \return Same socket instance
	/// 
	/// \throws NetworkError if the sock value indicates an error
	/// 
	/// \see Network::Check(ret_t, std::source_location), NetworkError
	static socket_t Check(
		socket_t sock,
		std::source_location loc = std::source_location::current()
	);
	
	/// \brief OS-independent socket close function
	///
	/// \param sock Socket instance to close
	/// 
	/// \throws NetworkError in case of error
	/// 
	/// Invokes closesocket() on windows and close() on unix systems.
	static void Close(
		socket_t sock,
		std::source_location loc = std::source_location::current()
	);

};

//========================================