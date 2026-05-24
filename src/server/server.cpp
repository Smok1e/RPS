#include <stdexcept>
#include <print>
#include <thread>
#include <iostream>

#include <server/server.hpp>
#include <server/client_handler.hpp>

//========================================

Server::Server(const Network* network, uint16_t port):
	m_network(network),
	m_port(port),
	m_socket(INVALID_SOCKET)
{
	sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	m_socket = m_network->check(socket(server_addr.sin_family, SOCK_STREAM, 0));

	m_network->check(
		bind(m_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr))
	);

	std::println("server initialized");
}

Server::~Server()
{
	m_network->check(closesocket(m_socket));
}

void Server::serve()
{
	m_network->check(listen(m_socket, 10));
	std::println("server listening on port {}", m_port);

	while (true)
	{
		addClient(m_network->check(accept(m_socket, nullptr, nullptr)));
		deleteRemovedClients();
	}
}

void Server::addClient(SOCKET socket)
{
	(new ClientHandler(this, socket))->start();
}

void Server::removeClient(ClientHandler* client)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_removed_clients.push_back(client);
}

void Server::deleteRemovedClients()
{
	for (auto* client: m_removed_clients)
		delete client;

	m_removed_clients.clear();
}

//========================================