#pragma once

#include <map>
#include <random>

#include <common/move.hpp>
#include <common/network.hpp>
#include <common/peer.hpp>

//========================================

/// \brief The main client class
///
/// The ClientPeer class, derived from Peer, handles communication with server
/// and minimal text user interface.
/// 
/// \see Peer
class ClientPeer: public Peer
{
public:
	/// \brief Constructor
	/// 
	/// \param socket The socket that is connected to a server
	ClientPeer(Network::socket_t socket);

	/// \brief The main ClientPeer procedure
	///
	/// This method will make ClientPeer into an ExpectingKeyExchangeInit state, 
	/// then call pollPacket or interpretCommand, depending on the state, in a loop 
	/// until the running flag is set to false.
	/// 
	/// It will catch any protocol related exceptions and send an error
	/// message to the remote peer in some cases. 
	/// 
	/// \see Peer::pollPacket, interpretCommand
	void start() override;

private:
	/// The state
	enum class State
	{
		Initial,                           //!< Initial state
		UserInput,                         //!< Expecting user to input a command
		ExpectingKeyExchangeInit,		   //!< Expecting KeyExchangeInit message
		ExpectingPlayerAuthorizationReply, //!< Expecting PlayerAuthorizationReply message
		ExpectingPlayerScoreReply,		   //!< Expecting PlayerScoreReply message
		ExpectingPlayerMoveReply,		   //!< Expeting PlayerMoveReply message
		ExpectingMoveHistoryReply,		   //!< Expecting MoveHistoryReply message
		ExpectingPerformanceTestReply	   //!< Expecting PerformanceTestReply message
	} m_state = State::Initial;

	std::minstd_rand m_random_generator;
	uint8_t m_performance_test_data[128] = {};
	std::chrono::high_resolution_clock::time_point m_performance_test_start_time = {};
	int m_performance_test_total_packets = 0;

	/// \brief The command handler table
	///
	/// This table compares user command and corresponding handler method.
	/// the interpretCommand method will use it to find suitable command handler.
	/// 
	/// \see interpretCommand
	static std::map<std::string_view, void(ClientPeer::*)()> s_command_handler_table;

	/// \brief Handles incoming message
	///
	/// \param id The message id
	/// 
	/// This method will handle all client related message types
	/// and call corresponding handler. If the message is not handled
	/// by the client, it calls its base variant.
	/// 
	/// \see Peer::MessageID, Peer::onMessageReceived
	void onMessageReceived(MessageID id) override;

	/// \brief KeyExchangeInit message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// When KeyExchangeInit message arrives from the server, this method
	/// sends client public key to the remote peer, and then calls deriveSharedSecret
	/// method to establish common encryption key. 
	/// 
	/// Then, the PlayerAuthorizationRequest is sent by the auth
	/// method.
	/// 
	/// \see Peer::deriveSharedSecret, auth
	void onKeyExchangeInit();

	/// \brief PlayerAuthorizationReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// If authorization is successful, calls startGame method. Otherwise,
	/// retries authorization by calling auth.
	/// 
	/// \see startGame, auth
	void onPlayerAuthorizationReply();

	/// \brief PlayerScoreReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Prints global and current player score table received from the server.
	/// After that, the ClientPeer goes back into a UserInput state.
	/// 
	/// \see State, sendPlayerScoreRequest
	void onPlayerScoreReply();

	/// \brief PlayerMoveReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Prints player and server moves and the game result. After that,
	/// the ClientPeer goes back into a UserInput state.
	/// 
	/// \see State, sendPlayerMoveRequest
	void onPlayerMoveReply();

	/// \brief MoveHistoryReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Prints move history received from the server.
	/// After that, the ClientPeer goes back into a UserInput state.
	/// 
	/// \see State, sendMoveHistoryRequest
	void onMoveHistoryReply();

	/// \brief PerformanceTestReply message handler
	///
	/// \throws ProtocolError in case the message is not expected
	/// 
	/// Validates the data returned by the remote peer. Then, if the
	/// test time does not exceed configured performance test duration, 
	/// calls sendPerformanceTestRequest method to send another test
	/// request. Otherwise, prints performance test summary and moves
	/// ClientPeer into UserInput state.
	/// 
	/// \see State, sendPerformanceTestRequest, config
	void onPerformanceTestReply();

	/// \brief Sends PlayerAuthorizationRequest message
	///
	/// \param username Player username
	/// \param password Player password
	/// \param _register If true, new user will be registered
	/// 
	/// The method will send PlayerAuthorizationRequest message to the
	/// server and put ClientPeer into ExpectingPlayerAuthorizationReply
	/// state.
	/// 
	/// \see State, onPlayerAuthorizationReply
	void sendPlayerAuthorizationRequest(
		std::string_view username,
		std::string_view password,
		bool _register
	);

	/// \brief Sends PlayerScoreRequest message
	///
	/// Sends PlayerScoreRequest message and puts ClientPeer into
	/// ExpectingPlayerScoreReply state.
	/// 
	/// \see State, onPlayerScoreReply
	void sendPlayerScoreRequest();

	/// \brief Sends PlayerMoveRequest message
	///
	/// \param move The player move
	/// 
	/// Sends PlayerMoveRequest message and puts ClientPeer into
	/// ExpectingPlayerMoveReply state.
	/// 
	/// \see State, onPlayerMoveReply
	void sendPlayerMoveRequest(GameMove move);

	/// \brief Sends MoveHistoryRequest message
	///
	/// Sends MoveHistoryRequest message and puts ClientPeer into
	/// ExpectingMoveHistoryReply state.
	/// 
	/// \see State, onMoveHistoryReply
	void sendMoveHistoryRequest();

	/// \brief Sends PerformanceTestRequest message
	///
	/// Fills m_performance_test_data with random bytes and sends
	/// them with the PerformanceTestRequest message. Then, the
	/// ClientPeer goes into ExpectingPerformanceTestReply state.
	/// 
	/// \see State, onPerformanceTestReply
	void sendPerformanceTestRequest();

	/// \brief Authorizes user
	///
	/// Asks user to input credentials, then passes them into the
	/// sendPlayerAuthorizationRequest method.
	/// 
	/// \see sendPlayerAuthorizationRequest
	void auth();

	/// \brief Starts game
	///
	/// This method will print available commands list as keys from the
	/// s_command_handle_table, then put the ClientPeer into a UserInput
	/// state.
	/// 
	/// \see State, s_command_handler_table
	void startGame();

	/// \brief Interprets user command
	///
	/// This method will read a single command from the user, then find
	/// and call corresponding handler in the s_command_handler_table.
	/// 
	/// If the command is unknown, the method returns.
	/// 
	/// \see s_command_handler_table
	void interpretCommand();

	void onScoreCommand();    //!< "score" command handler; calls sendPlayerScoreRequest()
	void onQuitCommand();	  //!< "quit" command handler; calls stop()
	void onRockCommand();	  //!< "rock" command handler; calls sendPlayerMoveRequest()
	void onPaperCommand();	  //!< "paper" command handler; calls sendPlayerMoveRequest()
	void onScissorsCommand(); //!< "scissors" command handler; calls sendPlayerMoveRequest()
	void onHistoryCommand();  //!< "history" command handler; calls sendMoveHistoryRequest()

	/// \brief "perf" command handler
	///
	/// This method initiates performance test by measuring current time
	/// and resetting packet counter. It then calls sendPerformanceTestRequest,
	/// which will trigger performance test packet sequence. After performance
	/// test is completed, the user receives test summary.
	/// 
	/// \see sendPerformanceTestRequest, onPerformanceTestReply
	void onPerfCommand();

};

//========================================