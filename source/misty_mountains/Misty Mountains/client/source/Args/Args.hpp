#ifndef ARGS_HPP
#define ARGS_HPP

#pragma once

#include <args.hpp>
#include <iostream>

class Args
{
public:
	/// <summary>
	/// Parsing Arguments from cmdline
	/// </summary>
	auto Initialize(const int argc, char* argv[]) -> void
	{
		args::ArgumentParser m_parser("## Misty Mountains ##", "TCP echo client.");

		args::HelpFlag m_help(m_parser, "help", "Display this help menu", { 'h', "help" });

		args::Group m_g_arguments(m_parser, "Arguments", args::Group::Validators::DontCare, args::Options::Global);

		args::ValueFlag<std::string> m_sz_hostname(m_g_arguments, "host", "Hostname to connect. By default 127.0.0.1.", { 'h', "host" });
		args::ValueFlag<std::string> m_sz_port(m_g_arguments, "port", "Port to connect. By default 1337.", { 'p', "port" });
		///

		try
		{
			m_parser.ParseCLI(argc, argv);

			this->m_sz_hostname_ = m_sz_hostname.Get();
			this->m_sz_port_ = m_sz_port.Get();
		}
		catch (const args::Help&)
		{
			std::cout << m_parser;
			std::exit(EXIT_SUCCESS);
		}
		catch (const args::ParseError& e)
		{
			std::cerr << e.what() << '\n';
			std::cerr << m_parser;
			std::exit(EXIT_FAILURE);
		}
	}

	auto Hostname(void) -> std::string&
	{
		return m_sz_hostname_;
	}
	
	auto Port(void) -> std::string&
	{
		return m_sz_port_;
	}
	
private:
	std::string m_sz_hostname_;
	std::string m_sz_port_;
};

#endif // !ARGS_HPP

