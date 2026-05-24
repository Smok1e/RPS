#include <print>
#include <iostream>

#include <network.hpp>
#include <server/server.hpp>

//========================================

int main()
{
	try
	{
		Network network;

		Server server(&network, 1488);
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