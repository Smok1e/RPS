#pragma once

#include <map>
#include <random>

#include <common/move.hpp>
#include <common/network.hpp>
#include <common/peer.hpp>

//========================================

class ClientPeer: public Peer
{
public:
	ClientPeer(Network::socket_t socket);

	void start() override;

private:
	enum class State
	{
		Initial,
		ExpectingKeyExchangeInit,
		ExpectingPlayerAuthorizationReply,
		ExpectingPlayerScoreReply,
		ExpectingPlayerMoveReply,
		ExpectingMoveHistoryReply,
		ExpectingPerformanceTestReply
	} m_state = State::Initial;

	std::minstd_rand m_random_generator;
	uint8_t m_performance_test_data[128] = {};
	std::chrono::high_resolution_clock::time_point m_performance_test_start_time = {};
	int m_performance_test_total_packets = 0;

	static std::map<std::string_view, void(ClientPeer::*)()> s_command_handler_table;

	void onMessageReceived(MessageID id) override;

	void onKeyExchangeInit();
	void onPlayerAuthorizationReply();
	void onPlayerScoreReply();
	void onPlayerMoveReply();
	void onMoveHistoryReply();
	void onPerformanceTestReply();

	void sendPlayerAuthorizationRequest();
	void sendPlayerScoreRequest();
	void sendPlayerMoveRequest(GameMove move);
	void sendMoveHistoryRequest();
	void sendPerformanceTestRequest();

	void startGame();
	void interpretCommand();

	void onScoreCommand();
	void onQuitCommand();
	void onRockCommand();
	void onPaperCommand();
	void onScissorsCommand();
	void onHistoryCommand();
	void onPerfCommand();

};

//========================================