#include <print>
#include <iostream>
#include <ranges>
#include <cassert>

#include <common/peer.hpp>
#include <common/config.hpp>

//========================================

MessageNotAllowedError::MessageNotAllowedError():
	ProtocolError("the peer does not handle this type of message")
{}

//========================================

Peer::Peer(Network::socket_t socket):
	m_socket(socket)
{
	// Retrieving remote address
	sockaddr_in addr = {};
	Network::socklen_t addr_len = sizeof(addr);

	Network::Check(
		getpeername(m_socket, reinterpret_cast<sockaddr*>(&addr), &addr_len)
	);

	m_remote_addr = std::format(
		"{}:{}",
		inet_ntoa(addr.sin_addr), 
		ntohs(addr.sin_port)
	);

	DEBUG_LOG("[{}]: connected", m_remote_addr);

	// Generating keypair
	auto* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
	EVP_PKEY_keygen_init(ctx);
	EVP_PKEY_keygen(ctx, &m_pkey);
	EVP_PKEY_CTX_free(ctx);

	DEBUG_LOG("[{}]: keypair generated", m_remote_addr);
}

Peer::~Peer()
{
	if (m_pkey)
		EVP_PKEY_free(m_pkey);

	if (m_cipher_in_ctx)
		EVP_CIPHER_CTX_free(m_cipher_in_ctx);

	if (m_cipher_out_ctx)
		EVP_CIPHER_CTX_free(m_cipher_out_ctx);

	Network::Close(m_socket);
}

void Peer::stop()
{
	DEBUG_LOG(
		"[{}]: stopping...",
		m_remote_addr
	);

	m_running = false;
}

//========================================

void Peer::pollPacket()
{
	char buffer[128] = "";

	auto len = Network::Check(recv(m_socket, buffer, sizeof(buffer), 0));
	if (!len)
	{
		std::println("[{}]: disconnected", m_remote_addr);		

		stop();
		return;
	}

	onDataReceived(reinterpret_cast<uint8_t*>(buffer), len);
}

void Peer::sendPacket()
{
	m_packet_out.seek(0, true, Packet::OffsetType::Writing);
	m_packet_out.writeUint16(m_packet_out.size());

	DEBUG_LOG(
		"[{}]: sending {} bytes:",
		m_remote_addr,
		m_packet_out.size()
	);

	DEBUG_DUMP_HEX(m_packet_out);

	if (m_cipher_out_ctx)
	{
		int encrypted_size = 0;
		EVP_CipherUpdate(
			m_cipher_in_ctx,
			m_packet_out.data(),
			&encrypted_size,
			m_packet_out.data(),
			m_packet_out.size()
		);

		assert(encrypted_size == m_packet_out.size());
	}

	Network::Check(
		send(
			m_socket, 
			reinterpret_cast<const char*>(m_packet_out.data()),
			m_packet_out.size(),
			0
		)
	);

	m_packet_out.clear();
}

void Peer::onDataReceived(const uint8_t* data, size_t size)
{
	DEBUG_LOG("[{}]: received {} bytes", m_remote_addr, size);

	if (m_cipher_in_ctx)
	{
		std::unique_ptr<uint8_t[]> decrypted(new uint8_t[size]);

		int decrypted_size = 0;
		EVP_CipherUpdate(
			m_cipher_in_ctx,
			decrypted.get(),
			&decrypted_size,
			data,
			size
		);

		assert(decrypted_size == size);
		m_packet_in.write(decrypted.get(), decrypted_size);
	}

	else
		m_packet_in.write(data, size);

	// Header is received, trying to read whole packet
	if (m_pending_packet_size)
	{
		if (m_packet_in.remain() < m_pending_packet_size)
		{
			DEBUG_LOG(
				"[{}]: packet still received partially (expecting {} more bytes)",
				m_remote_addr,
				m_pending_packet_size - m_packet_in.remain()
			);
			
			return;
		}
		
		processPacket(m_pending_packet_size);
		m_pending_packet_size = 0;
	}
	
	// Processing remaining data
	while (m_packet_in.remain() >= sizeof(uint16_t))
	{
		size_t packet_size = m_packet_in.readUint16();
		if (m_packet_in.remain() < packet_size)
		{
			m_pending_packet_size = packet_size;

			DEBUG_LOG(
				"[{}]: packet received partially (expecting %zu more bytes, {} total)",
				m_remote_addr,
				packet_size - m_packet_in.remain(),
				packet_size
			);

			return;
		}

		processPacket(packet_size);
	}
}

void Peer::processPacket(size_t packet_size)
{
	DEBUG_LOG(
		"[{}]: packet fully received ({} bytes):", 
		m_remote_addr, 
		packet_size
	);

	DEBUG_DUMP_HEX(m_packet_in);

	auto offset = m_packet_in.offset();
	onMessageReceived(m_packet_in.readGeneric<MessageID>());

	m_packet_in.seek(offset + packet_size, true);
	m_packet_in.discard();
}

//========================================

void Peer::onMessageReceived(MessageID id)
{
	switch (id)
	{
		case MessageID::DebugMessage:
			onDebugMessage();
			break;

		case MessageID::Error:
			onError();
			break;

		default:
			throw MessageNotAllowedError();

	}
}

void Peer::onDebugMessage()
{
	std::println(
		"[{}]: debug message received: {}", 
		m_remote_addr, 
		m_packet_in.readString()
	);
}

void Peer::onError()
{
	DEBUG_LOG(
		"[{}]: error message received",
		m_remote_addr
	);

	std::println(
		std::cerr, 
		"remote peer error: {}",
		m_packet_in.readString()
	);

	std::println(std::cerr, "aborting");
	stop();
}

//========================================

void Peer::sendPublicKey(MessageID message_id)
{
	assert(
		message_id == MessageID::KeyExchangeInit || 
		message_id == MessageID::KeyExchangeReply
	);

	DEBUG_LOG(
		"[{}]: sending KEX {}",
		m_remote_addr,
		message_id == MessageID::KeyExchangeInit
			? "init"
			: "reply"
	);

	curve25519_key_t public_key;
	size_t public_key_len = sizeof(public_key);
	EVP_PKEY_get_raw_public_key(m_pkey, public_key, &public_key_len);

	m_packet_out.writeGeneric(message_id);
	m_packet_out.writeGeneric(public_key);
	sendPacket();
}

void Peer::deriveSharedSecret()
{
	curve25519_key_t remote_public_key;
	m_packet_in.read(remote_public_key, sizeof(remote_public_key));

	auto* remote_pkey = EVP_PKEY_new_raw_public_key(
		EVP_PKEY_X25519, 
		nullptr, 
		remote_public_key, 
		sizeof(remote_public_key)
	);

	auto* ctx = EVP_PKEY_CTX_new(m_pkey, nullptr);

	EVP_PKEY_derive_init(ctx);
	EVP_PKEY_derive_set_peer(ctx, remote_pkey);

	size_t shared_secret_len = sizeof(m_shared_secret);
	EVP_PKEY_derive(ctx, m_shared_secret, &shared_secret_len);
	EVP_PKEY_CTX_free(ctx);

	DEBUG_LOG("[{}]: derived shared secret:", m_remote_addr);
	DEBUG_DUMP_HEX(m_shared_secret);

	auto* cipher = EVP_CIPHER_fetch(nullptr, config::cipher, nullptr);

	m_cipher_in_ctx = EVP_CIPHER_CTX_new();
	EVP_CipherInit(
		m_cipher_in_ctx, 
		cipher, 
		m_shared_secret, 
		m_shared_secret, 
		false
	);

	m_cipher_out_ctx = EVP_CIPHER_CTX_new();
	EVP_CipherInit(
		m_cipher_out_ctx, 
		cipher, 
		m_shared_secret, 
		m_shared_secret, 
		true
	);

	EVP_CIPHER_free(cipher);

	DEBUG_LOG("[{}]: cipher engaged", m_remote_addr);
}

void Peer::sendError(std::string_view message)
{
	m_packet_out.clear();
	m_packet_out.writeGeneric(MessageID::Error);
	m_packet_out.writeString(message);
	sendPacket();
}

//========================================

void DumpHex(std::span<const uint8_t> data)
{
	constexpr size_t line_size = 16;

	for (size_t offset = 0; offset < data.size(); offset += line_size)
	{
		std::print("{:04}: ", offset);
		
		for (size_t i = 0; i < line_size; i++)
		{
			if (offset + i >= data.size())
			{
				std::print("   ");
				continue;
			}
			
			std::print("{:02x} ", data[offset + i]);
		}
		
		std::print(" ");
		
		for (size_t i = 0; i < line_size && offset + i < data.size(); i++)
		{
			auto byte = data[offset + i];
			std::print(
				"{:c}",
				0x20 <= byte && byte <= 0x7E
					? byte
					: '.'
			);
		}
		
		std::println("");
	}
}

//========================================
