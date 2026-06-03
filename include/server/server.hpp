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

/// \brief Server class
/// 
/// The main server class that handles port binding, client accepting 
/// and holds an instance of the ScoreProvider class. 
/// 
/// When server accepts a client, it allocates an instance of ServerPeer friend, 
/// class, which receives newly connected socket and a pointer to server instance. 
/// It can then use this pointer to access the ScoreProvider instance to work with 
/// player database. 
/// 
/// The server does not store any pointers to the ServerPeer instances it creates
/// and does not handle its lifetime by itself. Instead, the ServerPeer instance
/// should pass its pointer to the Server::removeClient() method, which will store
/// it until the main thread finally deletes ServerPeer instance. 
/// 
/// serve() method starts server loop.
/// 
/// \note In order to safely access common server and ScoreProvider methods
/// across different threads, Server::m_sync_root mutex should be locked 
/// when entering critical sections. The server will do it on its own.
/// 
/// \see ServerPeer
class Server
{
public:
	friend class ServerPeer;

	/// \brief Constructor
	///
	/// \param port The port that server should listen to
	Server(uint16_t port);

	/// Destructor
	~Server();

	/// \brief Starts server main loop
	///
	/// This method will start port listening andthen
	/// accept any clients in an infinite loop.
	void serve();

private:
	uint16_t m_port;
	Network::socket_t m_socket = Network::InvalidSocket;
	std::vector<ServerPeer*> m_removed_clients;

	/// \brief Common server mutex
	///
	/// The synchronization root of all client threads. This should be
	/// locked by any client thread when entering a critical section.
	std::mutex m_sync_root;

	ScoreProvider m_score_provider;
	std::minstd_rand m_random_generator;

	/// \brief Allocates and starts a new client 
	///
	/// \param socket The client socket
	/// 
	/// This method will allocate a new ServerPeer instance and calls
	/// its start method. When client thread completes its execution,
	/// it should call Server::removeClient method in order to mark 
	/// its poitner for deletion.
	/// 
	/// \see removeClient
	void addClient(Network::socket_t socket);

	/// \brief Marks client pointer for deletion
	///
	/// \param client The pointer to the ServerPeer instance that should be deleted
	/// 
	/// This method simply appends a pointer to the m_removed_clients vector.
	/// The client instance is then deleted by main thread by calling 
	/// deleteRemovedClients method.
	/// 
	/// \see deleteRemovedClients
	void removeClient(ServerPeer* client);

	/// \brief Deletes all clients marked as removed
	///
	/// This method simply deletes all client instances that are stored in the
	/// m_removed_clients vector. This method should be only called by the main
	/// thread, as client deletion will also delete its thread.
	/// 
	/// \see removeClient
	void deleteRemovedClients();

	/// Generates random GameMove value
	GameMove generateMove();

};

//========================================