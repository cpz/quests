#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <mutex>

#include <kissnet.hpp>
namespace kn = kissnet;

using namespace std::chrono_literals;

#include "Configuration/Configuration.hpp"
#include "Args/Args.hpp"
#include "XML/XML.hpp"

//std::mutex g_lock;

auto main(const int argc, char* argv[]) -> int
{
	auto config = std::make_unique<Configuration>();
	auto args = std::make_unique<Args>();
	auto xml = std::make_unique<Xml>();
	
	args->Initialize(argc, argv);

	//Configuration (by default)
	config->Port(1337);

	//close program upon ctrl+c or other signals
	std::signal(SIGINT, [](int) {
		std::cout << "Got sigint signal...\n";
		std::exit(EXIT_SUCCESS);
		});

	// Initialize XML
	xml->Initialize( std::move( args->Path() ) );

	config->Port( std::move( xml->Port() ) );
	config->Prefix( std::move( xml->Prefix() ) );
	config->Suffix( std::move( xml->Suffix() ) );

	// if cmdline argument is not empty and xml argument is empty then using cmdline argument
	if (!args->Prefix().empty() && 
		config->Prefix().empty()) {
		config->Prefix( std::move( args->Prefix() ) );
		std::cout << "Using prefix " << config->Prefix() << " from cmdline" << '\n';
	}

	if (!args->Suffix().empty() && 
		config->Suffix().empty()) {
		config->Suffix( std::move(args->Suffix() ) );
		std::cout << "Using suffix " << config->Suffix() << " from cmdline" << '\n';
	}

	//If specified : get port from command line
	try
	{
		if (!args->Port().empty() &&
			config->TextPort().empty())
		{
			const auto port = kn::port_t(std::stoi(args->Port(), nullptr, 10));
			config->Port(port);
		}
		else {
			const auto port = kn::port_t(std::stoi(config->TextPort(), nullptr, 10));
			config->Port(port);
		}
	} catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
		std::cerr << "Wrong port variable" << '\n';
		std::exit(EXIT_FAILURE);
	}

	//We need to store thread objects somewhere
	std::vector<std::thread> threads;
	//We need to store socket objects somewhere
	std::vector<kn::tcp_socket> sockets;
	
	//Create a listening TCP socket on requested port
	kn::tcp_socket listen_socket({ "0.0.0.0", config->Port() });
	listen_socket.bind();
	listen_socket.listen();

	//Send the SIGINT signal to our self if user press return on "server" terminal
	std::thread run_th([] {
		std::cout << "press return to close server...\n";
		std::cin.get(); //This call only returns when user hit RETURN
		std::cin.clear();
		std::raise(SIGINT);
		});

	//Let that thread run alone
	run_th.detach();

	//Loop that continuously accept connections
	while (true)
	{
		std::cout << "Waiting for a client on port " << config->Port() << '\n';
		sockets.emplace_back(listen_socket.accept());
		auto& client = sockets.back();
		const auto client_info = client.get_recv_endpoint();

		//Create thread that will echo bytes received to the client
		threads.emplace_back([&] {
			std::cout << "Started thread for " << client_info.address << ':' << client_info.port << " (thread id: " << std::this_thread::get_id() << ") " << '\n';
			
			//Internal loop
			auto continue_receiving = true;

			kn::buffer<4096> static_buffer;
			
			//While connection is alive
			while (continue_receiving)
			{				
				//attept to receive data
				if (auto [data_size, valid] = client.recv(static_buffer); valid)
				{					
					if (valid.value == kn::socket_status::cleanly_disconnected)
					{
						continue_receiving = false;
					}
					else
					{
						if (data_size < static_buffer.size()) 
							static_buffer[data_size] = std::byte{ '\0' };
						
						std::string message(reinterpret_cast<const char*>(static_buffer.data()), data_size);
						
						std::cout << "Incoming from " << client_info.address << ":" << client_info.port << '\n';
						std::cout << "Data: " << message << '\n';

						// add prefix if non empty
						if (!config->Prefix().empty()) {
							message.insert(0, config->Prefix());
						}

						// add suffix if non empty
						if (!config->Suffix().empty()) {
							message.insert(message.length(), config->Suffix());
						}
						
						auto* const data_byte = reinterpret_cast<const std::byte*>(message.c_str());
						const auto data_length = message.length();
						
						client.send(data_byte, data_length);
					}
				}
				//If not valid remote host closed connection
				else
				{
					continue_receiving = false;
				}
			}

			//Now that we are outside the loop, erase this socket from the "sockets" list:
			std::cout << "detected disconnect from " << client_info.address << ':' << client_info.port << " (thread id: " << std::this_thread::get_id() << ") " << '\n';
			if (const auto me_iterator = std::find(sockets.begin(), sockets.end(), std::ref(client)); me_iterator != sockets.end())
			{
				std::cout << "closing socket...\n";
				sockets.erase(me_iterator);
			}
			});

		threads.back().detach();
	}

	return EXIT_SUCCESS;
}
