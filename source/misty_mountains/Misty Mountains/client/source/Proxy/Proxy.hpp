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

		this->src_host_ = proxy_host;
		this->src_port_ = port_t;

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
			status = http_handshake();
			break;
		case Protocol::kSocks4:
			status = socks4_handshake();
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

	auto get(void) -> kissnet::tcp_socket*
	{
		return &this->s_proxy_;
	}

	template <size_t buff_size>
	auto send(const kissnet::buffer<buff_size>& buff, const size_t length = buff_size) -> std::tuple<size_t, kissnet::socket_status>
	{
		return this->s_proxy_.send(buff, length);
	}

	auto send(const std::byte* read_buff, size_t length) -> std::tuple<size_t, kissnet::socket_status>
	{
		return this->s_proxy_.send(read_buff, length);
	}

	template <size_t buff_size>
	auto recv(kissnet::buffer<buff_size>& write_buff, size_t start_offset = 0) -> std::tuple<size_t, kissnet::socket_status>
	{
		return this->s_proxy_.recv(write_buff, start_offset);
	}

	auto recv(std::byte* buffer, size_t len, bool wait = true) -> std::tuple<size_t, kissnet::socket_status>
	{
		return this->s_proxy_.recv(buffer, len, wait);
	}

private:
	auto socks5_handshake() -> bool
	{
		kissnet::buffer<4096> buffer;

		enum Version : char
		{
			kSock4 = 4,
			kSock5 = 5,
		};
		
		enum Type : char
		{
			kConnect = 1,
			kBind,
			kUdpAssociate
		};

		enum Auth : char
		{
			kNone = 0, // No authentication
			kGSSAPI,
			kAuth, // Username/password
			kChallengeHandshake,
			kUnassigned,
			kChallengeResponce,
			kSSL,
			kNDS,
			kMAF,
			kJSON,
			kFailed = 0xFF
		};

		enum Address : char
		{
			kIPv4 = 1, 
			kDomain = 3,
			kIPv6 = 4
		};

		enum Reply : char
		{
			kRequestGranted = 0,
			kGeneralFailure,
			kConnectionNotAlloed,
			kNetworkUnreachable,
			kHostUnreachable,
			kConnectionRefused,
			kTTL,
			kProtocolError,
			kAddresType
		};
		
		struct AuthenticationRequest
		{
			char ver;
			char nauth;
			char auth[255];
		} auth_req {};

		auth_req.ver = kSock5;
		auth_req.nauth = 2;			// Authentication methods
		auth_req.auth[0] = kNone;
		auth_req.auth[1] = kAuth;

		auto byte = reinterpret_cast<const std::byte*>(&auth_req);
		auto [auth_s_size, auth_s_valid] = this->s_proxy_.send(byte, 4);
		if (auth_s_valid != kissnet::socket_status::valid)
			return false;

		struct AuthenticationAnswer
		{
			char ver;
			char cauth;
		} auth_ans {};

		auto [auth_r_size, auth_r_valid] = this->s_proxy_.recv(buffer);
		if (auth_r_valid != kissnet::socket_status::valid)
			return false;

		auth_ans.ver = static_cast<int>(buffer[0]);
		if (auth_ans.ver != kSock5)
			return false;
		
		auth_ans.cauth = static_cast<int>(buffer[1]);
		if (auth_ans.cauth == kFailed) {
			std::cerr << "Can't authorizate, no acceptable methods were offered." << '\n';
			return false;
		}
		
		if(auth_ans.cauth == kAuth) {
			struct ClientRequest
			{
				char ver;
				char idlen;
				char id[255];
				char pwlen;
				char pw[255];
			} client_req{};

			struct ClientAnswer
			{
				char ver;
				char status;
			} client_ans{};

			/*
			if (client_ans.status != 0x00)
				return false;
			*/
			
			std::cerr << "Authorization via username and password arent supported yet." << '\n';
			return false;
		}

		struct ConnectionRequest
		{
			char ver;
			char cmd;
			char rsv;
			char type;
			unsigned long	dest_address;
			unsigned short	dest_port;
		} con_req {};
		
		con_req.ver = kSock5;
		con_req.cmd = kConnect;
		con_req.rsv = 0x00;
		con_req.type = kIPv4;
		con_req.dest_address = this->Ip();
		con_req.dest_port = ntohs(this->dest_port_);

		if (con_req.dest_address == INADDR_NONE)
			return false;

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

		return con_ans.reply == kRequestGranted;
	}

	auto socks4_handshake() -> bool
	{
		kissnet::buffer<4096> buffer;

		enum Version : char
		{
			kSock4 = 4,
			kSock5 = 5
		};

		enum Type : char
		{
			kConnect = 1,
			kBind
		};

		enum Reply : char
		{
			kRequestGranted = 90,
			kRequestFailed,
			kRequestRejectedIdentd,
			kRequestRejectedUserid
		};

		struct AuthenticationRequest
		{
			char vn;
			char cd;
			unsigned short dest_port;
			unsigned long dest_address;
			char userid[256];
		} auth_req{};

		auth_req.vn = kSock5;
		auth_req.cd = kConnect;
		auth_req.dest_port = ntohs(this->dest_port_);
		auth_req.dest_address = this->Ip();
		auth_req.userid[0] = '\0';

		if (auth_req.dest_address == INADDR_NONE)
			return false;

		const auto byte = reinterpret_cast<const std::byte*>(&auth_req);
		auto [auth_s_size, auth_s_valid] = this->s_proxy_.send(byte, 4);
		if (auth_s_valid != kissnet::socket_status::valid)
			return false;

		struct AuthenticationAnswer
		{
			char vn;
			char cd;
			unsigned short dest_port;
			unsigned long dest_address;
		} auth_ans{};

		auto [auth_r_size, auth_r_valid] = this->s_proxy_.recv(buffer);
		if (auth_r_valid != kissnet::socket_status::valid)
			return false;

		auth_ans.vn = static_cast<int>(buffer[0]);
		if (auth_ans.vn != kSock4)
			return false;

		auth_ans.cd = static_cast<int>(buffer[1]);

		return auth_ans.cd == kRequestGranted;
	}

	auto http_handshake() -> bool
	{
		auto status{ false };
		const auto host = std::string{ this->src_host_ + ':' + std::to_string(this->src_port_) };
		
		if (this->username_.empty() &&
			this->password_.empty()) {
			const auto request = std::string{ "CONNECT" + host +  " HTTP/1.1\r\nHost: " + host + "\r\n\r\n" };

			auto [s_size, s_status] = this->s_proxy_.send(reinterpret_cast<const std::byte*>(request.c_str()), request.size());
			if(s_status != kissnet::socket_status::valid) {
				std::cerr << "Can't connect to " << host << '\n';
				return status;
			}
		} else {
			const auto auth = this->username_ + ':' + this->password_;
			const auto b64 = this->Encode(auth);
			const auto request = std::string{ "CONNECT" + host + " HTTP/1.1\r\nHost: " + host + "r\nAuthorization: Basic " + b64 +"\r\nProxy-Authorization: Basic " + b64 + "\r\n\r\n" };

			auto [s_size, s_status] = this->s_proxy_.send(reinterpret_cast<const std::byte*>(request.c_str()), request.size());
			if (s_status != kissnet::socket_status::valid) {
				std::cerr << "Can't connect to " << host << '\n';
				return status;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		kissnet::buffer<1024> buffer;
		auto [r_size, r_status] = this->s_proxy_.recv(buffer);
		if (r_status != kissnet::socket_status::valid) {
			std::cerr << "Cant receive data from proxy" << '\n';
			return status;
		}

		if (r_size < buffer.size())
			buffer[r_size] = std::byte{ '\0' };
		
		const auto back = std::string(reinterpret_cast<const char*>(buffer.data()), r_size);
		if (back.find("HTTP/1.1 200") != std::string::npos)
			status = true;
		
		return status;
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

	/// https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594
	static std::string Encode(const std::string value) {
		static constexpr char s_encoding_table[] = {
		  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		  'w', 'x', 'y', 'z', '0', '1', '2', '3',
		  '4', '5', '6', '7', '8', '9', '+', '/'
		};

		const auto in_len = value.size();
		const auto out_len = 4 * ((in_len + 2) / 3);
		std::string ret(out_len, '\0');
		size_t i;
		auto p = const_cast<char*>(ret.c_str());

		for (i = 0; i < in_len - 2; i += 3) {
			*p++ = s_encoding_table[(value[i] >> 2) & 0x3F];
			*p++ = s_encoding_table[((value[i] & 0x3) << 4) | ((int)(value[i + 1] & 0xF0) >> 4)];
			*p++ = s_encoding_table[((value[i + 1] & 0xF) << 2) | ((int)(value[i + 2] & 0xC0) >> 6)];
			*p++ = s_encoding_table[value[i + 2] & 0x3F];
		}
		if (i < in_len) {
			*p++ = s_encoding_table[(value[i] >> 2) & 0x3F];
			if (i == (in_len - 1)) {
				*p++ = s_encoding_table[((value[i] & 0x3) << 4)];
				*p++ = '=';
			}
			else {
				*p++ = s_encoding_table[((value[i] & 0x3) << 4) | ((int)(value[i + 1] & 0xF0) >> 4)];
				*p++ = s_encoding_table[((value[i + 1] & 0xF) << 2)];
			}
			*p++ = '=';
		}

		return ret;
	}
	
private:
	kissnet::tcp_socket s_proxy_;

	std::string src_host_;
	uint16_t src_port_{};
	
	std::string dest_host_;
	uint16_t dest_port_{};

	std::string username_;
	std::string password_;
};

#endif // !PROXY_HPP
