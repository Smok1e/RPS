#pragma once

#include <thread>

#include <common/score_provider.hpp>
#include <common/network.hpp>
#include <common/peer.hpp>

//========================================

class Server;

/// The client instance
///
/// The ServerPeer class (also just referred as "client" in terms of server part) is
/// a server context of a connected client. Derived from Peer, it stores a pointer
/// to the Server instance in order to have access to shared resources. Its main 
/// loop is executed in an own separate thread in order to support simultanious handling
/// of multiple clients.
/// 
/// \note Any shared access to the server fields or methods should be performed in 
/// a critical section by locking Server::m_sync_root mutex.
/// 
/// \see Server
class ServerPeer: public Peer
{
public:
	/// \brief Constructor
	///
	/// Initially, m_thread is created as an empty thread. To start execution,
	/// call the start() method.
	///
	/// \see start
	ServerPeer(Network::socket_t socket, Server* server);

	/// Destructor
	~ServerPeer();

	/// \brief Start client handling loop 
	///
	/// This method starts a separate thread which executes threadProc 
	/// method.
	/// 
	/// \see threadProc
	void start() override;

private:
	/// The state
	enum class State
	{
		Initial,                            //!< The initial state
		ExpectingKeyExchangeReply,			//!< The ServerPeer is expecting a KeyExchangeReply message 
		ExpectingPlayerAhtorizationRequest,	//!< The ServerPeer is expecting a PlayerAuthorizationRequest message

		/// The ServerPeer is expecting any of the following messages: 
		///	- PlayerScoreRequest
		/// - PlayerMoveRequest
		/// - MoveHistoryRequest
		/// - PerformanceTestRequest
		/// 
		/// or any other messages that the basic Peer is able to handle.
		ExpectingCommand					
	} m_state = State::Initial;

	ScoreProvider::PlayerID m_player_id = ScoreProvider::InvalidPlayer;
	ScoreProvider::PlayerScore m_current_score = {};
	std::vector<std::pair<GameMove, GameMove>> m_move_history;

	Server* m_server;
	std::thread m_thread;

	/// \brief The main ServerPeer procedure
	///
	/// This method will initiate a key exchange by sending public key
	/// to the remote peer, then call pollPacket in a loop until the 
	/// running flag is set to false by calling stop() method.
	/// 
	/// It will catch any protocol related exceptions and send an error
	/// message to the remote peer in some cases. 
	void threadProc();

	/// \brief Handles incoming message
	///
	/// \param id The message id
	/// 
	/// This method will handle all server related message types
	/// and call corresponding handler. If the message is not handled
	/// by the server, it calls its base variant.
	/// 
	/// \see Peer::MessageID, Peer::onMessageReceived
	void onMessageReceived(MessageID id) override;

	/// \brief KeyExchangeReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// When KeyExchangeReply message arrives, when it is expected,
	/// this method derives shared secret by calling the deriveSharedSecret.
	/// 
	/// After key exchange is completed, and client-server communication is
	/// encrypted, the ServerPeer expects a PlayerAuthorizationRequest message.
	/// 
	/// \see Peer::deriveSharedSecret
	void onKeyExchangeReply();

	/// \brief PlayerAuthorizationRequest message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// When PlayerAuthorizationRequest message arrives, when it is expected,
	/// this method tries to find player id by its username in the database,
	/// then validates its password, and calls sendPlayerAuthorizationReply.
	/// 
	/// If the authorization succeedes, the ServerPeer goes into an ExpectingCommand
	/// state, in which it will accept any further game messages. Otherwise, it remains
	/// in an ExpectingPlayerAuthorizationRequest state.
	/// 
	/// \see ScoreProvider::findPlayer, ScoreProvider::checkPlayerPassword, sendPlayerAuthorizationReply
	void onPlayerAuthorizationRequest();

	/// \brief PlayerScoreRequest message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Simply calls the sendPlayerScoreReply method.
	/// 
	/// \see sendPlayerScoreReply
	void onPlayerScoreRequest();

	/// \brief PlayerMoveRequest message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// When PlayerMoveRequest message arrives, this method calls Server::generateMove method
	/// to generate server move, compares moves, appends them to the m_move_history buffer
	/// and sends a PlayerMoveReply message by calling sendPlayerMoveReply method.
	/// 
	/// \see Server::generateMove, ScoreProvider::updatePlayerScore, sendPlayerMoveResposne
	void onPlayerMoveRequest();

	/// \brief MoveHistoryRequest message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Simply calls the sendMoveHistoryReply method.
	/// 
	/// \see sendMoveHistoryReply
	void onMoveHistoryRequest();

	/// \brief PerformanceTestReuqest message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Simply calls the sendPerformanceTestReply method.
	/// 
	/// \see sendPerformanceTestReply
	void onPerformanceTestRequest();

	/// \brief Sends PlayerAuthorizationReply message
	///
	/// \param success The flag that indicates whether authorization is successful or not
	/// \param reason Authorization failure reason
	void sendPlayerAuthorizationReply(bool success, std::string_view reason = "");

	/// \brief Sends PlayerScoreReply message
	///
	/// This method retrieves player score by calling ScoreProvider::getPlayerScore
	/// and sends it to the remote peer.
	/// 
	/// \see ScoreProvider::getPlayerScore
	void sendPlayerScoreReply();

	/// \brief Sends PlayerMoveReply message
	///
	/// \param player_move The player move
	/// \param server_move The server move
	///	\param result The game result
	void sendPlayerMoveReply(GameMove player_move, GameMove server_move, GameResult result);

	/// Sends MoveHistoryReply message
	void sendMoveHistoryReply();

	/// Sends PerformanceTestReply message
	void sendPerformanceTestReply();

};

//========================================