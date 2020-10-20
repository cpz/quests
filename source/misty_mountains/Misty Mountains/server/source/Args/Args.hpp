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
		args::ArgumentParser m_parser("## Misty Mountains ##", "TCP echo server.");

		args::HelpFlag m_help(m_parser, "help", "Display this help menu", { 'h', "help" });

		args::Group m_g_arguments(m_parser, "Arguments", args::Group::Validators::DontCare, args::Options::Global);

		args::ValueFlag<std::string> m_sz_port(m_g_arguments, "port", "Port to listen. By default 1337", { 'p', "port" });
		args::ValueFlag<std::string> m_sz_path(m_g_arguments, "xml", "Path to XML Configuration file. By default 'config.xml' near executable.", { 'x', "xml" });
		args::ValueFlag<std::string> m_sz_prefix(m_g_arguments, "prefix", "Prefix to add to echo. By default none.", { 'f', "prefix" });
		args::ValueFlag<std::string> m_sz_suffix(m_g_arguments, "suffix", "Suffix to add to echo. By default none.", { 's', "suffix" });
		///

		try
		{
			m_parser.ParseCLI(argc, argv);

			this->m_sz_port_ = m_sz_port.Get();
			this->m_sz_path_ = m_sz_path.Get();
			this->m_sz_prefix_ = m_sz_prefix.Get();
			this->m_sz_suffix_ = m_sz_suffix.Get();
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

	auto Port(void) -> std::string&
	{
		return m_sz_port_;
	}

	auto Path(void) -> std::string&
	{
		return m_sz_path_;
	}

	auto Prefix(void) -> std::string&
	{
		return m_sz_prefix_;
	}

	auto Suffix(void) -> std::string&
	{
		return m_sz_suffix_;
	}
	
private:
	std::string m_sz_port_;
	std::string m_sz_path_;
	std::string m_sz_prefix_;
	std::string m_sz_suffix_;
};

#endif // !ARGS_HPP

