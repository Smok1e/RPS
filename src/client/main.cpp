#include <print>
#include <iostream>

#include <ArgParser/ArgParser.hpp>

#include <common/config.hpp>
#include <client/client_peer.hpp>

//========================================

int main(int argc, char* argv[])
{
	try
	{
		ArgParser options {
			{"help",    "Print usage reference and exit",     },
			{"address", "Override server address",        true},
			{"port",    "Override server port",           true}
		};

		options.parse(argc, argv);

		if (options["help"])
		{
			std::cout << options << std::endl;
			return 0;
		}

		Network network;

		const char* server_addr = options["address"](config::default_address);
		uint16_t server_port = options["port"](config::default_port);

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(server_port);

		if (!inet_pton(
			addr.sin_family,
			server_addr,
			&addr.sin_addr
		))
			throw std::invalid_argument("invalid server address");

		auto sock = Network::Check(
			socket(addr.sin_family, SOCK_STREAM, 0)
		);

		std::println("Connecting to {}:{}...", server_addr, server_port);
		Network::Check(
			connect(
				sock, 
				reinterpret_cast<sockaddr*>(&addr), 
				sizeof(addr)
			)
		);

		ClientPeer peer(sock);
		peer.start();
	}

	catch (const NetworkError& err)
	{
		std::println("Network error: {}", err.getMessage());
	}

	catch (const std::exception& exc)
	{
		std::println("{}", exc.what());
	}
}

//========================================