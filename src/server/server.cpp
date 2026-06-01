#include <stdexcept>
#include <print>
#include <thread>
#include <iostream>

#include <server/server.hpp>
#include <server/server_peer.hpp>

//========================================

Server::Server(uint16_t port):
	m_port(port)
{
	sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	m_socket = Network::Check(socket(server_addr.sin_family, SOCK_STREAM, 0));

	Network::Check(
		bind(
			m_socket, 
			reinterpret_cast<sockaddr*>(&server_addr), 
			sizeof(server_addr)
		)
	);

	std::random_device device;
	m_random_generator = std::minstd_rand(device());

	std::println("server initialized");
}

Server::~Server()
{
	Network::Check(closesocket(m_socket));
}

void Server::serve()
{
	Network::Check(listen(m_socket, 10));
	std::println("server listening on port {}", m_port);

	while (true)
	{
		addClient(Network::Check(accept(m_socket, nullptr, nullptr)));
		deleteRemovedClients();
	}
}

void Server::addClient(SOCKET socket)
{
	(new ServerPeer(socket, this))->start();
}

void Server::removeClient(ServerPeer* client)
{
	m_removed_clients.push_back(client);
}

void Server::deleteRemovedClients()
{
	for (auto* client: m_removed_clients)
		delete client;

	m_removed_clients.clear();
}

GameMove Server::generateMove()
{
	std::uniform_int_distribution<int> distribution(
		0,
		static_cast<int>(GameMove::Amount) - 1
	);

	return static_cast<GameMove>(distribution(m_random_generator));
}

//========================================