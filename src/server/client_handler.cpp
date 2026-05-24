#include <print>
#include <iostream>

#include <server/server.hpp>
#include <server/client_handler.hpp>

//========================================

ClientHandler::ClientHandler(Server* server, SOCKET socket):
	m_server(server),
	m_socket(socket)
{
	sockaddr_in addr = {};
	int addr_len = sizeof(addr);

	m_server->m_network->check(
		getpeername(m_socket, reinterpret_cast<sockaddr*>(&addr), &addr_len)
	);

	m_address = std::format(
		"{}:{}",
		inet_ntoa(addr.sin_addr), 
		ntohs(addr.sin_port)
	);

	std::println("[{}]: client accepted", m_address);
}

ClientHandler::~ClientHandler()
{
	if (m_thread.joinable())
		m_thread.join();

	m_server->m_network->check(closesocket(m_socket));
}

void ClientHandler::start()
{
	m_thread = std::thread(&ClientHandler::threadProc, this);
}

void ClientHandler::threadProc()
{
	try
	{
		std::println("[{}]: handler thread started", m_address);

		char buffer[128] = "";
		int len = sizeof(buffer);

		while ((len = m_server->m_network->check(recv(m_socket, buffer, len, 0))) > 0)
			onDataReceived(reinterpret_cast<uint8_t*>(buffer), len);

		std::println("[{}]: connection has been gracefully closed", m_address);
	}

	catch (const NetworkError& err)
	{
		std::println(
			std::cerr, "[{}]: network error: {}", 
			m_address, 
			err.getMessage()
		);
	}

	m_server->removeClient(this);
}

//========================================

void ClientHandler::onDataReceived(const uint8_t* data, size_t size)
{
	m_incoming_packet.write(data, size);

#ifdef NETWORK_VERBOSE
		std::println("received {} bytes", size);
#endif

	if (m_pending_packet_length)
	{
		if (m_incoming_packet.remain() >= m_pending_packet_length)
		{
			onPacketReceived(m_pending_packet_length);
			m_pending_packet_length = 0;
		}
		
		else
		{
#ifdef NETWORK_VERBOSE
			std::println(
				"packet still received partially (expecting {} more bytes)",
				m_pending_packet_length - m_incoming_packet.remain()
			);
#endif
			
			return;
		}
	}
	
	while (m_incoming_packet.remain() >= 4)
	{
		size_t packet_length = m_incoming_packet.readUint32();
		
		if (m_incoming_packet.remain() >= packet_length)
			onPacketReceived(packet_length);
		
		else
		{
#ifdef NETWORK_VERBOSE
			std::println(
				"packet received partially (expecting {} more bytes, {} total)",
				packet_length - m_incoming_packet.remain(),
				packet_length
			);
#endif
			
			m_pending_packet_length = packet_length;
			return;
		}
	}	
}

void ClientHandler::onPacketReceived(size_t size)
{
#ifdef NETWORK_VERBOSE
	std::println("received full packet ({} bytes)", size);
#endif

	auto offset = m_incoming_packet.offset();

	std::println("received message: {}", m_incoming_packet.readString());

	m_incoming_packet.seek(offset + size, true);
	m_incoming_packet.discard();
}

//========================================