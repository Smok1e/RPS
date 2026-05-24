#include <stdexcept>
#include <print>

#include <network.hpp>

//========================================

NetworkError::NetworkError(std::source_location loc /*= std::source_location::current()*/):
	m_loc(loc)
{
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
	WSAData wsa_data = {};
	if (WSAStartup(0x0202, &wsa_data) != 0)
		throw std::runtime_error("WSAStartup failed");

	std::println("network initialized");
}

Network::~Network()
{
	WSACleanup();

	std::println("network uninitialized");
}

int Network::check(int ret, std::source_location loc /*= std::source_location::current()*/) const
{
	if (ret == SOCKET_ERROR)
		throw NetworkError(loc);

	return ret;
}

SOCKET Network::check(SOCKET sock, std::source_location loc /*= std::source_location::current()*/) const
{
	if (sock == INVALID_SOCKET)
		throw NetworkError(loc);

	return sock;
}

//========================================