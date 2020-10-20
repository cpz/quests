#ifndef PROXY_HPP
#define PROXY_HPP

#include <iostream>
#include <kissnet.hpp>

/// proxy types
enum class Protocol {
	kHttp,
	kSocks4,
	kSocks5
};

class Proxy
{
public:
	auto Initialize(const std::string&& proxy_host, std::string&& proxy_port,
	                std::string&& dest_host, const uint16_t dest_port, 
		std::string&& username = "", std::string&& password = "") -> bool
	{
		auto status{ false };
		kissnet::port_t port_t = 0;
		
		if (proxy_host.empty() || proxy_port.empty() ||
			dest_host.empty() || !dest_port)
			return status;

		try
		{
			const auto p = std::stoi(proxy_port, nullptr, 10);
			port_t = kissnet::port_t(p);
		}
		catch (const std::exception& e) {
			std::cerr << e.what() << '\n';
			return false;
		}

		const auto endpoint = kissnet::endpoint{ proxy_host, port_t  };
		
		kissnet::tcp_socket s_proxy(endpoint);
		
		s_proxy_ = std::move(s_proxy);
		if (s_proxy_.is_valid() &&
			s_proxy_.connect() == kissnet::socket_status::valid)
				status = true;
		
		this->dest_host_ = dest_host;
		this->dest_port_ = dest_port;

		if(!username.empty() &&
			!password.empty()) {
			this->username_ = username;
			this->password_ = password;
		}
		
		return status;
	}
	
	auto Connect(Protocol type = Protocol::kSocks5) -> bool
	{
		auto status{ false };

		switch(type) {
		case Protocol::kHttp:
		case Protocol::kSocks4:
			break;
		case Protocol::kSocks5:
			status = socks5_handshake();
			break;
		default:
			assert(false);
			break;
		}
		
		return status;
	}

	auto Get(void) -> kissnet::tcp_socket*
	{
		return &this->s_proxy_;
	}

private:
	auto socks5_handshake() -> bool
	{
		kissnet::buffer<4096> buffer;

		struct AuthenticationRequest
		{
			char version;
			char method;
			char methods[255];
		} auth_req {};

		auth_req.version = 0x05;			// socks5
		auth_req.method = 0x02;			// numbers of methods
		auth_req.methods[0] = 0x00;			// NO AUTHENTICATION REQUIRED
		auth_req.methods[1] = 0x02;			// USERNAME/PASSWORD

		auto byte = reinterpret_cast<const std::byte*>(&auth_req);
		auto [auth_s_size, auth_s_valid] = this->s_proxy_.send(byte, 4);
		if (auth_s_valid != kissnet::socket_status::valid)
			return false;

		struct AuthenticationAnswer
		{
			char version;
			char method;
		} auth_ans {};

		auto [auth_r_size, auth_r_valid] = this->s_proxy_.recv(buffer);
		if (auth_r_valid != kissnet::socket_status::valid)
			return false;

		auth_ans.version = static_cast<int>(buffer[0]);
		if (auth_ans.version != 5)
			return false;
		
		auth_ans.method = static_cast<int>(buffer[1]);
		if(auth_ans.method == 2) {
			std::cerr << "Authorization via username and password arent supported yet." << '\n';
			return false;
		}

		struct ConnectionRequest
		{
			char version;
			char command;
			char reserved;
			char address_type;
			unsigned long	dest_address;
			unsigned short	dest_port;
		} con_req {};
		
		con_req.version = 0x05; // socks version (u can use auth_ans.version)
		con_req.command = 0x01; // CONNECT X'01', BIND X'02', UDP ASSOCIATE X'03'
		con_req.reserved = 0x00; // RESERVED
		con_req.address_type = 0x01; // IP V4 address: X'01', DOMAINNAME: X'03', IP V6 address: X'04'
		con_req.dest_address = this->Ip();
		con_req.dest_port = ntohs(this->dest_port_);

		byte = reinterpret_cast<const std::byte*>(&con_req);
		auto [con_s_size, con_s_valid] = this->s_proxy_.send(byte, 10);
		if (con_s_valid != kissnet::socket_status::valid)
			return false;

		struct ConnectionAnswer
		{
			char version;
			char reply;
			char reserved;
			char address_type;
			char other[256];
		} con_ans {};

		auto [con_r_size, con_r_valid] = this->s_proxy_.recv(buffer);
		if (con_r_valid != kissnet::socket_status::valid)
			return false;

		con_ans.version = static_cast<int>(buffer[0]);
		con_ans.reply = static_cast<int>(buffer[1]);

		return con_ans.reply == 0; // X'00' succeeded
	}

	auto Ip(const char* hostname) -> unsigned long
	{
		unsigned long ret = INADDR_NONE;
		
		if (hostname)
		{
			ret = inet_addr(hostname);
			if (ret == INADDR_NONE)
			{
				auto* const host = gethostbyname(hostname);
				if (host == nullptr)
					return INADDR_NONE;

				RtlCopyMemory(&ret, host->h_addr_list[0], host->h_length);
			}
		}
		return ret;
	}

	auto Ip() -> unsigned long
	{
		return Ip(this->dest_host_.c_str());
	}
	
private:
	kissnet::tcp_socket s_proxy_;

	std::string dest_host_;
	uint16_t dest_port_{};

	std::string username_;
	std::string password_;
};

#endif // !PROXY_HPP
