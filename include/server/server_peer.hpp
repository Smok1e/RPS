#pragma once

#include <thread>

#include <common/score_provider.hpp>
#include <common/network.hpp>
#include <common/peer.hpp>

//========================================

class Server;

class ServerPeer: public Peer
{
public:
	ServerPeer(Network::socket_t socket, Server* server);
	~ServerPeer();

	void start();

private:
	enum class State
	{
		Initial,
		ExpectingKeyExchangeReply,
		ExpectingPlayerAhtorizationRequest,
		ExpectingCommand
	} m_state = State::Initial;

	ScoreProvider::PlayerID m_player_id = ScoreProvider::InvalidPlayer;
	ScoreProvider::PlayerScore m_current_score = {};
	std::vector<std::pair<GameMove, GameMove>> m_move_history;

	Server* m_server;
	std::thread m_thread;

	void threadProc();

	void onMessageReceived(MessageID id) override;
	void onKeyExchangeReply();
	void onPlayerAuthorizationRequest();
	void onPlayerScoreRequest();
	void onPlayerMoveRequest();
	void onMoveHistoryRequest();
	void onPerformanceTestRequest();

	void sendPlayerAuthorizationReply(bool success);
	void sendPlayerScoreReply();
	void sendPlayerMoveReply(GameMove player_move, GameMove server_move, GameResult result);
	void sendMoveHistoryReply();
	void sendPerformanceTestReply();

};

//========================================