#include <iostream>
#include <thread>
#include <cstddef>
#include <csignal>

using namespace std::chrono_literals;

#include <kissnet.hpp>
namespace kn = kissnet;

#include "Args/Args.hpp"
#include "Proxy/Proxy.hpp"

auto main(const int argc, char* argv[]) -> int
{
	auto args = std::make_unique<Args>();
	auto proxy = std::make_unique<Proxy>();

	args->Initialize(argc, argv);
	
	//Configuration (by default)
	kn::port_t port = 1337;
	std::string hostname{ "127.0.0.1" };

	if (!args->Hostname().empty())
	{
		hostname = args->Hostname();
	}

	try
	{
		if (!args->Port().empty())
		{
			const auto p = std::stoi(args->Port(), nullptr, 10);
			port = kn::port_t(p);
		}
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		std::cerr << "Wrong port variable" << '\n';
		std::exit(EXIT_FAILURE);
	}


	/// SOCKS5 Proxy Example
	if (proxy->Initialize("127.0.0.1", "1488",
		"127.0.0.1", 1337) &&
		proxy->Connect(Protocol::kSocks5)) {
		std::cout << "Connected to proxy!" << '\n';

		auto hello_world = "Hello World!";
		auto* const byte = reinterpret_cast<const std::byte*>(hello_world);
		auto const length = strlen(hello_world);

		auto [size, status] = proxy->send(byte, length);
		if (status == kn::socket_status::valid) {
			std::cout << "Hello world was sent! Size: " << size << '\n';
		}
		else {
			std::cout << "Something went wrong with proxy send" << '\n';
		}
	}

	kn::tcp_socket sv_sock({ hostname, port });

	if (!sv_sock.connect()) {
		std::cout << "Error connecting to server at  " << hostname << ':' << port << '\n';
		std::this_thread::sleep_for(2s);
		std::exit(EXIT_FAILURE);
	}

	sv_sock.set_non_blocking(true);

	//close program upon ctrl+c or other signals
	std::signal(SIGINT, [](int) {
		std::cout << "Got sigint signal...\n";
		std::exit(0);
		});

	std::cout << "Connected to " << hostname << " on port " << port << '\n';

	//Read user data into temp buffer
	std::string message;
	uint64_t req_id = 0;

	while (true) {
		std::cout << ">> ";
		std::getline(std::cin, message);

		if (!message.compare("quit") ||
			message.empty()) {
			break;
		}

		auto start = std::chrono::high_resolution_clock::now();

		auto* const data_byte = reinterpret_cast<const std::byte*>(message.c_str());
		const auto data_length = message.length();

		// Send the data that buffer contains
		auto [send_size, send_status] = sv_sock.send(data_byte, data_length);
		if (!send_size || send_status != kissnet::socket_status::valid)
		{
			std::cout << "Cannot send message to server" << '\n';
			std::raise(SIGINT);
		}

		std::this_thread::sleep_for(1s);

		const auto bytes_available = sv_sock.bytes_available();
		std::cout << "Bytes available: " << bytes_available << '\n';

		kn::buffer<4096> buffer;

		//Get the data, and the lengh of data
		auto [recv_size, recv_status] = sv_sock.recv(buffer);
		if (!recv_size || recv_status != kissnet::socket_status::valid)
		{
			std::cout << "Cannot recv message from server" << '\n';
			std::raise(SIGINT);
		}

		if (recv_size < buffer.size())
			buffer[recv_size] = std::byte{ '\0' };

		std::string recieved(reinterpret_cast<const char*>(buffer.data()), recv_size);

		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = now - start;
		printf("handled request #%llu. handled %2.2f r/q in %2.2fs.\n\r\nData: %s\r\nSize: %i\r\n", req_id,
			(float)((double)req_id / elapsed.count()), (float)elapsed.count(), recieved.c_str(), recv_size);

		req_id++;
	}

	return EXIT_SUCCESS;
}
