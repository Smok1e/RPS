#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <random>

#include <common/move.hpp>
#include <common/network.hpp>
#include <common/score_provider.hpp>

//========================================

class ServerPeer;

class Server
{
public:
	friend class ServerPeer;

	Server(uint16_t port);
	~Server();

	void serve();

private:
	uint16_t m_port;
	Network::socket_t m_socket = Network::InvalidSocket;
	std::vector<ServerPeer*> m_removed_clients;
	std::mutex m_mutex;
	ScoreProvider m_score_provider;
	std::minstd_rand m_random_generator;

	void addClient(Network::socket_t socket);
	void removeClient(ServerPeer* client);
	void deleteRemovedClients();
	GameMove generateMove();

};

//========================================