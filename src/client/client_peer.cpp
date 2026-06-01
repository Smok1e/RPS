#include <stdexcept>
#include <print>
#include <iostream>
#include <ranges>
#include <limits>

#include <common/score_provider.hpp>
#include <client/client_peer.hpp>

//========================================

ClientPeer::ClientPeer(SOCKET socket):
	Peer(socket)
{
	std::random_device device;
	m_random_generator = std::minstd_rand(device());
}

//========================================

void ClientPeer::start()
{
	try
	{
		DEBUG_LOG("[{}]: client started", m_remote_addr);
		m_state = State::ExpectingKeyExchangeInit;

		while (m_running)
			pollPacket();
	}

	catch (const ProtocolError& err)
	{
		DEBUG_LOG(
			"[{}]: protocol error: {}",
			m_remote_addr,
			err.what()
		);

		sendError(err.what());
	}

	catch (const NetworkError& err)
	{
		std::println(
			std::cerr, 
			"[{}]: network error: {}", 
			m_remote_addr, 
			err.getMessage()
		);
	}

	catch (const std::exception& exc)
	{
		std::println(
			std::cerr, 
			"[{}]: {}",
			m_remote_addr,
			exc.what()
		);
	}
}
						
//========================================

void ClientPeer::onMessageReceived(MessageID id)
{
	switch (id)
	{
		case MessageID::KeyExchangeInit:
			onKeyExchangeInit();
			break;

		case MessageID::PlayerAuthorizationReply:
			onPlayerAuthorizationReply();
			break;

		case MessageID::PlayerScoreReply:
			onPlayerScoreReply();
			break;

		case MessageID::PlayerMoveReply:
			onPlayerMoveReply();
			break;

		case MessageID::MoveHistoryReply:
			onMoveHistoryReply();
			break;

		case MessageID::PerformanceTestReply:
			onPerformanceTestReply();
			break;

		default:
			return Peer::onMessageReceived(id);

	}
}

void ClientPeer::onKeyExchangeInit()
{
	if (m_state != State::ExpectingKeyExchangeInit)
		throw ProtocolError("unexpected KeyExchangeInit received");

	DEBUG_LOG(
		"[{}]: KEX init received",
		m_remote_addr
	);

	sendPublicKey(MessageID::KeyExchangeReply);
	deriveSharedSecret();
	sendPlayerAuthorizationRequest();
}

void ClientPeer::onPlayerAuthorizationReply()
{
	if (m_state != State::ExpectingPlayerAuthorizationReply)
		throw ProtocolError("unexpected PlayerAuthorizationReply");

	auto success = m_packet_in.readBoolean();

	DEBUG_LOG(
		"[{}]: player authorization reply ({}) received",
		m_remote_addr,
		success
			? "success"
			: "failure"
	);

	if (success)
		startGame();

	else
	{
		std::println("Invalid username or password; please try again");
		sendPlayerAuthorizationRequest();
	}
}

void ClientPeer::onPlayerScoreReply()
{
	if (m_state != State::ExpectingPlayerScoreReply)
		throw ProtocolError("unexpected PlayerScoreReply");

	DEBUG_LOG(
		"[{}]: player score reply received",
		m_remote_addr
	);

	auto print_score = [&](std::string_view title) {
		std::println("=== {} ===", title);
		std::println("Wins:    {}", m_packet_in.readUint16());
		std::println("Defeats: {}", m_packet_in.readUint16());
		std::println("Draws:   {}", m_packet_in.readUint16());
		std::println("");
	};

	print_score("Global score");
	print_score("Session score");
	interpretCommand();
}

void ClientPeer::onPlayerMoveReply()
{
	if (m_state != State::ExpectingPlayerMoveReply)
		throw ProtocolError("unexpected PlayerMoveReply");

	DEBUG_LOG(
		"[{}]: player move reply received",
		m_remote_addr
	);

	auto player_move = m_packet_in.readGeneric<GameMove>();
	auto server_move = m_packet_in.readGeneric<GameMove>();
	auto result = m_packet_in.readGeneric<GameResult>();

	std::println(
		"{} vs {} => {}!", 
		std::to_string(player_move), 
		std::to_string(server_move), 
		std::to_string(result)
	);

	interpretCommand();
}

void ClientPeer::onMoveHistoryReply()
{
	if (m_state != State::ExpectingMoveHistoryReply)
		throw ProtocolError("unexpected MoveHistoryReply");

	DEBUG_LOG(
		"[{}]: move history reply received",
		m_remote_addr
	);

	auto history_size = m_packet_in.readUint16();
	if (history_size)
	{
		std::println("=== Session history ===");
		std::println("     {:10s} {:10s} {:10s}", "player", "server", "result");

		for (size_t i = history_size; i > 0; i--)
		{
			auto player_move = m_packet_in.readGeneric<GameMove>();
			auto server_move = m_packet_in.readGeneric<GameMove>();

			std::println(
				"[{:2d}] {:10s} {:10s} {:10s}", 
				i,
				std::to_string(player_move),
				std::to_string(server_move),
				std::to_string(player_move <=> server_move)
			);
		}
	}

	else
		std::println("history is empty");

	interpretCommand();
}

void ClientPeer::onPerformanceTestReply()
{
	if (m_state != State::ExpectingPerformanceTestReply)
		throw ProtocolError("unexpected PerformanceTestReply");

	m_performance_test_total_packets++;

	auto data_size = m_packet_in.readUint16();
	if (data_size != sizeof(m_performance_test_data))
	{
		std::println("performance test failed: invalid data size");
		interpretCommand();
		return;
	}

	if (memcmp(m_packet_in.data() + m_packet_in.offset(), m_performance_test_data, data_size) != 0)
	{
		std::println("performance test failed: different data");
		interpretCommand();
		return;
	}

	auto current_time = std::chrono::high_resolution_clock::now();
	auto test_duration = current_time - m_performance_test_start_time;
	if (test_duration > config::performance_test_duration)
	{
		std::println(
			"performance test completed in {}: {} packets received ({}/packet)", 
			std::chrono::duration_cast<std::chrono::milliseconds>(test_duration),
			m_performance_test_total_packets,
			std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
				test_duration / m_performance_test_total_packets
			)
		);

		interpretCommand();
		return;
	}

	sendPerformanceTestRequest();
}

//========================================

void ClientPeer::sendPlayerAuthorizationRequest()
{
	std::string username, password;

	std::print("Enter username: ");
	std::cin >> username;
	
	std::print("Enter password: ");
	std::cin >> password;

	DEBUG_LOG(
		"[{}]: sending player authorization request",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::PlayerAuthorizationRequest);
	m_packet_out.writeString(username);
	m_packet_out.writeString(password);
	sendPacket();
	
	m_state = State::ExpectingPlayerAuthorizationReply;
}

void ClientPeer::sendPlayerScoreRequest()
{
	DEBUG_LOG(
		"[{}]: sending player score request",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::PlayerScoreRequest);
	sendPacket();

	m_state = State::ExpectingPlayerScoreReply;
}

void ClientPeer::sendPlayerMoveRequest(GameMove move)
{
	DEBUG_LOG(
		"[{}]: sending player move request",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::PlayerMoveRequest);
	m_packet_out.writeGeneric(move);
	sendPacket();

	m_state = State::ExpectingPlayerMoveReply;
}

void ClientPeer::sendMoveHistoryRequest()
{
	DEBUG_LOG(
		"[{}]: sending move history request",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::MoveHistoryRequest);
	sendPacket();

	m_state = State::ExpectingMoveHistoryReply;
}

void ClientPeer::sendPerformanceTestRequest()
{
	std::uniform_int_distribution<int> dist(0x00, 0xFF);

	for (auto& elem: m_performance_test_data)
		elem = dist(m_random_generator);

	m_packet_out.writeGeneric(MessageID::PerformanceTestRequest);
	m_packet_out.writeUint16(sizeof(m_performance_test_data));
	m_packet_out.write(m_performance_test_data, std::size(m_performance_test_data));
	sendPacket();

	m_state = State::ExpectingPerformanceTestReply;
}

//========================================

std::map<std::string_view, void(ClientPeer::*)()> ClientPeer::s_command_handler_table = {
	{ "quit",     &ClientPeer::onQuitCommand      },
	{ "score",    &ClientPeer::onScoreCommand     },
	{ "history",  &ClientPeer::onHistoryCommand   },
	{ "rock",     &ClientPeer::onRockCommand      },
	{ "paper",    &ClientPeer::onPaperCommand     },
	{ "scissors", &ClientPeer::onScissorsCommand  },
	{ "perf",     &ClientPeer::onPerfCommand      }
};

void ClientPeer::startGame()
{
	std::println("Available commands:");
	for (const auto& [command, _]: s_command_handler_table)
		std::println("* {}", command);

	interpretCommand();
}

void ClientPeer::interpretCommand()
{
	std::string command;

	retry:
	std::print("> ");
	std::cin >> command;

	if (!s_command_handler_table.contains(command))
	{
		std::println("Unknown command");
		goto retry;
	}

	(this->*(s_command_handler_table[command]))();
	return;
}

void ClientPeer::onScoreCommand()
{
	sendPlayerScoreRequest();
}

void ClientPeer::onQuitCommand()
{
	stop();
}

void ClientPeer::onRockCommand()
{
	sendPlayerMoveRequest(GameMove::Rock);
}

void ClientPeer::onPaperCommand()
{
	sendPlayerMoveRequest(GameMove::Paper);
}

void ClientPeer::onScissorsCommand()
{
	sendPlayerMoveRequest(GameMove::Scissors);
}

void ClientPeer::onHistoryCommand()
{
	sendMoveHistoryRequest();
}

void ClientPeer::onPerfCommand()
{
	m_performance_test_start_time = std::chrono::high_resolution_clock::now();
	m_performance_test_total_packets = 0;

	std::println("starting performance test...");
	sendPerformanceTestRequest();
}

//========================================
