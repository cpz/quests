#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#pragma once

#include <string>

class Configuration
{
public:
	auto Port(const std::uint16_t value) -> void
	{
		ui_port_ = value;
	}

	auto Port(std::string&& value) -> void
	{
		sz_port_ = value;
	}

	auto Prefix(std::string&& value) -> void
	{
		sz_prefix_ = value;
	}

	auto Suffix(std::string&& value) -> void
	{
		sz_suffix_ = value;
	}

	auto Port(void) -> std::uint16_t
	{
		return ui_port_;
	}

	auto TextPort(void) -> const std::string&
	{
		return sz_port_;
	}

	auto Prefix(void) -> const std::string&
	{
		return sz_prefix_;
	}

	auto Suffix(void) -> const std::string&
	{
		return sz_suffix_;
	}

private:
	std::uint16_t ui_port_ = 1337;
	std::string sz_prefix_;
	std::string sz_suffix_;
	std::string sz_port_;
};

#endif // !CONFIGURATION_HPP
#define CONFIGURATION_HPP
