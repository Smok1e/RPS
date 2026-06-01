#pragma once

#include <string>
#include <stdexcept>
#include <span>

#include <common/network.hpp>
#include <common/packet.hpp>

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>

//========================================

constexpr size_t CURVE25519_KEY_LENGTH = 32;
using curve25519_key_t = uint8_t[CURVE25519_KEY_LENGTH];

//========================================

class ProtocolError: public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class MessageNotAllowedError: public ProtocolError
{
public:
	MessageNotAllowedError();

};

//========================================

class Peer
{
public:
	enum class MessageID: uint8_t
	{
		DebugMessage,
		Error,
		KeyExchangeInit,
		KeyExchangeReply,
		PlayerAuthorizationRequest,
		PlayerAuthorizationReply,
		PlayerScoreRequest,
		PlayerScoreReply,
		PlayerMoveRequest,
		PlayerMoveReply,
		MoveHistoryRequest,
		MoveHistoryReply,
		PerformanceTestRequest,
		PerformanceTestReply
	};

	Peer(Network::socket_t socket);
	~Peer();

	virtual void start() = 0;
	virtual void stop();

protected:
	bool m_running = true;
	std::string m_remote_addr;
	Network::socket_t m_socket;
	Packet m_packet_in;
	Packet m_packet_out;
	size_t m_pending_packet_size = 0;

	EVP_PKEY* m_pkey = nullptr;
	curve25519_key_t m_shared_secret = {};

	EVP_CIPHER_CTX* m_cipher_in_ctx = nullptr;
	EVP_CIPHER_CTX* m_cipher_out_ctx = nullptr;

	void pollPacket();
	void sendPacket();

	void onDataReceived(const uint8_t* data, size_t len);
	void processPacket(size_t packet_size);

	virtual void onMessageReceived(MessageID id);
	virtual void onDebugMessage();
	virtual void onError();

	void sendPublicKey(MessageID message_id);
	void deriveSharedSecret();
	void sendError(std::string_view message);

};

//========================================

void DumpHex(std::span<const uint8_t> data);

//========================================