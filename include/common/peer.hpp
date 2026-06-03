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

/// The length of key defined by the x25519 key exchange algorithm
constexpr size_t X25519_KEY_LENGTH = 32;             

/// x25519 key value type
using x25519_key_t = uint8_t[X25519_KEY_LENGTH];

//========================================

/// \brief Protocol error exception
///
/// Should be thrown in case of protocol errors (invalid packet number, wrong state, etc.)
class ProtocolError: public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

/// \brief Message not allowed exception
///
/// Specific type of protocol error, indicating that the received type of message
/// is not handled by the peer.
class MessageNotAllowedError: public ProtocolError
{
public:
	/// Default constructor
	MessageNotAllowedError();

};

//========================================

/// \brief Abstract peer class
///
/// This class is indended to handle common protocol communication
/// procedures, such as receiving and sending packets, handling universal
/// message types and connection encryption. The derived class should override
/// the onMessageReceived method to add custom message handling. In case the
/// derived class does not handle the message, it should call the 
/// Peer::onMessageReceived method for the default message handling.
class Peer
{
public:
	/// Protocol message IDs
	enum class MessageID: uint8_t
	{
		DebugMessage,               //<! Debug message (sent by client/server)
		Error,						//<! Error message (sent by client/server)
		KeyExchangeInit,			//<! x25519 key exchange init (sent only by server)
		KeyExchangeReply,			//<! x25519 key exchange reply (sent only by client)
		PlayerAuthorizationRequest,	//<! Player authorization request (sent only by client)
		PlayerAuthorizationReply,	//<! Player authorization reply (sent only by server)
		PlayerScoreRequest,			//<! Player score request (sent only by client)
		PlayerScoreReply,			//<! Player score reply (sent only by server)
		PlayerMoveRequest,			//<! Player move request (sent only by client)
		PlayerMoveReply,			//<! Player move reply (sent only by server)
		MoveHistoryRequest,			//<! Move history request (sent only by client)
		MoveHistoryReply,			//<! Move history reply (sent only by server)
		PerformanceTestRequest,		//<! Performance test request (sent only by client)
		PerformanceTestReply		//<! Performance test reply (sent only by server)
	};

	/// Constructor
	Peer(Network::socket_t socket);

	/// Destructor
	~Peer();

	/// Pure start method
	virtual void start() = 0;

	/// Sets running flag to false
	virtual void stop();

protected:
	bool m_running = true;
	std::string m_remote_addr;
	Network::socket_t m_socket;
	Packet m_packet_in;
	Packet m_packet_out;
	size_t m_pending_packet_size = 0;

	EVP_PKEY* m_pkey = nullptr;
	x25519_key_t m_shared_secret = {};

	EVP_CIPHER_CTX* m_cipher_in_ctx = nullptr;
	EVP_CIPHER_CTX* m_cipher_out_ctx = nullptr;

	/// \brief Reads available data from the socket and calls 
	/// onDataReceived when data arrives.
	/// 
	/// When remote peer disconnects, it calls the stop() method
	/// and returns.
	/// 
	/// \see stop, onDataReceived
	void pollPacket();

	/// \brief Sends output packet to the remote peer
	///
	/// After packet has been sent, it is cleared.
	void sendPacket();

	/// \brief Processes data arrived from the remote peer
	///
	/// \param data The data to process
	/// \param len Data length
	/// 
	/// If the cipher is engaged, this method first decrypts the data,
	/// then writes it into the incoming packet buffer. It tries to parse
	/// packet length and then waits for the whole packet to arrive. When
	/// packet is fully received, processPacket method is invoked with the
	/// received packet length.
	/// 
	/// \see processPacket
	void onDataReceived(const uint8_t* data, size_t len);

	/// \brief Processes packet when it is arrived
	///
	/// \param packet_size The size of the currently processed packet
	/// 
	/// This method reads message id from the packet header and passes it
	/// to onMessageReceived method. After onMessageReceived is done processing
	/// the packet, the packet is truncated until next packet beginning.
	/// 
	/// \see onMessageReceived
	void processPacket(size_t packet_size);

	/// \brief The default message handling procedure
	///
	/// \param id The message id
	/// \throws MessageNotAllowedError
	/// 
	/// Handles basic types of messages, such as DebugMessage or Error.
	/// In case the message is not common for the server and client,
	/// MessageNotAllowedError is thrown.
	/// 
	/// \see onDebugMessage, onError
	virtual void onMessageReceived(MessageID id);

	virtual void onDebugMessage(); //!< Debug message handler
	virtual void onError();        //!< Error message handler

	/// \brief Sends key exchange message to the remote peer
	///
	/// \param message_id KeyExchangeInit or KeyExchangeReply
	/// 
	/// Sends KeyExchangeInit/KeyExchangeReply message containing
	/// the peer's x25519 public key.
	/// 
	/// \see deriveSharedSecret
	void sendPublicKey(MessageID message_id);

	/// \brief Derives shared secret
	///
	/// Reads public key from the incoming packet buffer and derives
	/// shared secret using own private key and peer's public key.
	/// After the method is successfully invoked, all packets sent by
	/// the peer will be encrypted with the shared secret used as a key
	/// according to encryption algorithm.
	/// 
	/// \see sendPublicKey
	void deriveSharedSecret();

	/// \brief Sends error message to the remote peer
	///
	/// \param message The message to be sent
	void sendError(std::string_view message);

};

//========================================

/// \brief Dumps given data to the stdout in hex
///
/// \param data The data to be printed
void DumpHex(std::span<const uint8_t> data);

//========================================