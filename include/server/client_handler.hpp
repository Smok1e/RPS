#pragma once

#include <thread>

#include <network.hpp>
#include <packet.hpp>

//========================================

class Server;

class ClientHandler
{
public:
	ClientHandler(Server* server, SOCKET socket);
	~ClientHandler();

	void start();

private:
	Server* m_server;
	SOCKET m_socket;
	std::string m_address;
	std::thread m_thread;

	Packet m_incoming_packet;
	size_t m_pending_packet_length = 0;

	void threadProc();

	void onDataReceived(const uint8_t* data, size_t size);
	void onPacketReceived(size_t size);

};

//========================================