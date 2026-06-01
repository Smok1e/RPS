#include <stdexcept>
#include <print>

#include <common/network.hpp>

//========================================

NetworkError::NetworkError(std::source_location loc /*= std::source_location::current()*/):
	m_loc(loc)
{
#ifdef NETWORK_WINDOWS
	char* buffer = nullptr;
	size_t len = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM     | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&buffer),
		0,
		nullptr
	);

	if (len > 0 && buffer[len - 1] == '\n')
		len--;

	m_message = std::string(buffer, len);
	LocalFree(buffer);
#else
	m_message = strerror(errno);
#endif // NETWORK_WINDOWS
}

const std::source_location& NetworkError::getLocation() const
{
	return m_loc;
}

const std::string& NetworkError::getMessage() const
{
	return m_message;
}

//========================================


Network::Network()
{
#ifdef RPS_WINDOWS
	WSAData wsa_data = {};
	if (WSAStartup(0x0202, &wsa_data) != 0)
		throw std::runtime_error("WSAStartup failed");
#endif // RPS_WINDOWS
	
	DEBUG_LOG("network initialized");
}

Network::~Network()
{
#ifdef RPS_WINDOWS
	WSACleanup();
#endif // RPS_WINDOWS

	DEBUG_LOG("network uninitialized");
}

//========================================

Network::ret_t Network::Check(
	ret_t ret,
	std::source_location loc /*= std::source_location::current()*/
)
{
#ifdef RPS_WINDOWS
	if (ret == SOCKET_ERROR)
		throw NetworkError(loc);
#else
	if (ret == -1)
		throw NetworkError(loc);
#endif // RPS_WINDOWS

	return ret;
}

Network::socket_t Network::Check(
	socket_t sock,
	std::source_location loc /*= std::source_location::current()*/
)
{
#ifdef RPS_WINDOWS
	if (sock == INVALID_SOCKET)
		throw NetworkError(loc);
#else
	
	if (sock == -1)
		throw NetworkError(loc);
#endif // RPS_WINDOWS

	return sock;
}

void Network::Close(
	Network::socket_t sock,
	std::source_location loc /*= std::source_location::current()*/
)
{
#ifdef RPS_WINDOWS
	Check(closesocket(sock));
#else
	Check(close(sock), loc);
#endif // RPS_WINDOWS
}

//========================================