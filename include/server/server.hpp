#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <list>

#include <network.hpp>

//========================================

class Server
{
public:
	friend class ClientHandler;

	Server(const Network* network, uint16_t port);
	~Server();

	void serve();

private:
	const Network* m_network;
	uint16_t m_port;
	SOCKET m_socket;
	std::vector<ClientHandler*> m_removed_clients;
	std::mutex m_mutex;

	void addClient(SOCKET socket);
	void removeClient(ClientHandler* client);
	void deleteRemovedClients();

};

//========================================