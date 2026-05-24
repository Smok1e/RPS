#include <print>
#include <iostream>

#include <ArgParser/ArgParser.hpp>

#include <network.hpp>
#include <server/server.hpp>

//========================================

int main(int argc, char* argv[])
{
	try
	{
		ArgParser options {
			{"help", "Print usage reference and exit",     },
			{"port", "Override server port",           true}
		};

		options.parse(argc, argv);

		if (options["help"])
		{
			std::cout << options << std::endl;
			return 0;
		}

		Network network;

		Server server(&network, options["port"].as<int>(1488));
		server.serve();
	}

	catch (const NetworkError& err)
	{
		std::println(
			std::cerr, 
			"unexpected error in {} at {}:{}: {}", 
			err.getLocation().function_name(),
			err.getLocation().file_name(),
			err.getLocation().line(),
			err.getMessage()
		);
	}

	catch (const std::exception& exc)
	{
		std::println(std::cerr, "{}", exc.what());
	}
}

//========================================