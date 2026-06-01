#include <print>
#include <iostream>
#include <ranges>

#include <server/server.hpp>
#include <server/server_peer.hpp>

//======================================== 

ServerPeer::ServerPeer(SOCKET socket, Server* server):
	Peer(socket),
	m_server(server)
{}

ServerPeer::~ServerPeer()
{
	if (m_thread.joinable())
		m_thread.join();
}

void ServerPeer::start()
{
	m_thread = std::thread(&ServerPeer::threadProc, this);
}

void ServerPeer::threadProc()
{
	try
	{
		std::println("[{}]: handler thread started", m_remote_addr);

		sendPublicKey(MessageID::KeyExchangeInit);
		m_state = State::ExpectingKeyExchangeReply;

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

	std::lock_guard<std::mutex> lock(m_server->m_mutex);
	m_server->removeClient(this);
}

//========================================

void ServerPeer::onMessageReceived(MessageID id)
{
	switch (id)
	{
		case MessageID::KeyExchangeReply:
			onKeyExchangeReply();
			break;

		case MessageID::PlayerAuthorizationRequest:
			onPlayerAuthorizationRequest();
			break;

		case MessageID::PlayerScoreRequest:
			onPlayerScoreRequest();
			break;

		case MessageID::PlayerMoveRequest:
			onPlayerMoveRequest();
			break;

		case MessageID::MoveHistoryRequest:
			onMoveHistoryRequest();
			break;

		case MessageID::PerformanceTestRequest:
			onPerformanceTestRequest();
			break;

		default:
			Peer::onMessageReceived(id);

	}
}

void ServerPeer::onKeyExchangeReply()
{
	if (m_state != State::ExpectingKeyExchangeReply)
		throw ProtocolError("unexpected KeyExchangeReply received");

	DEBUG_LOG(
		"[{}]: KEX reply received",
		m_remote_addr
	);

	deriveSharedSecret();
	m_state = State::ExpectingPlayerAhtorizationRequest;
}

void ServerPeer::onPlayerAuthorizationRequest()
{
	if (m_state != State::ExpectingPlayerAhtorizationRequest)
		throw ProtocolError("unexpected PlayerAhtorizationRequest received");

	DEBUG_LOG(
		"[{}]: player authorization request received",
		m_remote_addr
	);

	auto username = m_packet_in.readString();
	auto password = m_packet_in.readString();

	ScoreProvider::PlayerID player_id;
	{
		std::lock_guard<std::mutex> lock(m_server->m_mutex);
		player_id = m_server->m_score_provider.findPlayer(username);
	}

	if (player_id == ScoreProvider::InvalidPlayer)
	{
		DEBUG_LOG(
			"[{}]: player unauthorized: no such player",
			m_remote_addr
		);

		sendPlayerAuthorizationReply(false);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_server->m_mutex);
		if (!m_server->m_score_provider.checkPlayerPassword(player_id, password))
		{
			DEBUG_LOG(
				"[{}]: player unauthorized: wrong password",
				m_remote_addr
			);

			sendPlayerAuthorizationReply(false);
			return;
		}
	}

	m_player_id = player_id;
	DEBUG_LOG(
		"[{}]: player authorized (id = {}); waiting for commands",
		m_remote_addr,
		player_id
	);

	sendPlayerAuthorizationReply(true);
	m_state = State::ExpectingCommand;
}

void ServerPeer::onPlayerScoreRequest()
{
	if (m_state != State::ExpectingCommand)
		throw ProtocolError("unexpected PlayerScoreRequest received");

	DEBUG_LOG(
		"[{}]: player score request received",
		m_remote_addr
	);

	sendPlayerScoreReply();
}

void ServerPeer::onPlayerMoveRequest()
{
	if (m_state != State::ExpectingCommand)
		throw ProtocolError("unexpected PlayerMoveRequest received");

	DEBUG_LOG(
		"[{}]: player move request received",
		m_remote_addr
	);

	auto player_move = m_packet_in.readGeneric<GameMove>();
	GameResult result;

	GameMove server_move;
	{
		std::lock_guard<std::mutex> lock(m_server->m_mutex);
		server_move = m_server->generateMove();

		result = player_move <=> server_move;
		m_server->m_score_provider.updatePlayerScore(m_player_id, result);
	}

	DEBUG_LOG(
		"[{}]: player / server: {} / {} => {}",
		m_remote_addr,
		std::to_string(player_move),
		std::to_string(server_move),
		std::to_string(result)
	);

	m_current_score += result;
	m_move_history.emplace_back(player_move, server_move);

	sendPlayerMoveReply(player_move, server_move, result);
}

void ServerPeer::onMoveHistoryRequest()
{
	DEBUG_LOG(
		"[{}]: move history request received",
		m_remote_addr
	);

	sendMoveHistoryReply();
}

void ServerPeer::onPerformanceTestRequest()
{
	sendPerformanceTestReply();
}

//========================================

void ServerPeer::sendPlayerAuthorizationReply(bool success)
{
	DEBUG_LOG(
		"[{}]: sending player authorization reply ({})",
		m_remote_addr,
		success
			? "success"
			: "failure"
	);

	m_packet_out.writeGeneric(MessageID::PlayerAuthorizationReply);
	m_packet_out.writeBoolean(success);
	sendPacket();
}

void ServerPeer::sendPlayerScoreReply()
{
	DEBUG_LOG(
		"[{}]: sending player score",
		m_remote_addr
	);

	ScoreProvider::PlayerScore score;
	{
		std::lock_guard<std::mutex> lock(m_server->m_mutex);
		score = m_server->m_score_provider.getPlayerScore(m_player_id);
	};

	m_packet_out.writeGeneric(MessageID::PlayerScoreReply);
	m_packet_out.writeUint16(score.wins);
	m_packet_out.writeUint16(score.defeats);
	m_packet_out.writeUint16(score.draws);
	m_packet_out.writeUint16(m_current_score.wins);
	m_packet_out.writeUint16(m_current_score.defeats);
	m_packet_out.writeUint16(m_current_score.draws);
	sendPacket();
}

void ServerPeer::sendPlayerMoveReply(GameMove player_move, GameMove server_move, GameResult result)
{
	DEBUG_LOG(
		"[{}]: sending player move reply",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::PlayerMoveReply);
	m_packet_out.writeGeneric(player_move);
	m_packet_out.writeGeneric(server_move);
	m_packet_out.writeGeneric(result);
	sendPacket();	

}

void ServerPeer::sendMoveHistoryReply()
{
	DEBUG_LOG(
		"[{}]: sending move history reply",
		m_remote_addr
	);

	m_packet_out.writeGeneric(MessageID::MoveHistoryReply);
	m_packet_out.writeUint16(m_move_history.size());
	for (const auto& [player_move, server_move]: m_move_history | std::views::reverse)
	{
		m_packet_out.writeGeneric(player_move);
		m_packet_out.writeGeneric(server_move);
	}

	sendPacket();
}

void ServerPeer::sendPerformanceTestReply()
{
	auto data_size = m_packet_in.readUint16();
	m_packet_out.writeGeneric(MessageID::PerformanceTestReply);
	m_packet_out.writeUint16(data_size);
	m_packet_out.write(m_packet_in.data() + m_packet_in.offset(), data_size);
	sendPacket();
}

//========================================