#ifndef XML_HPP
#define XML_HPP

#pragma once

#include <csignal>
#include <Windows.h>
#include <tinyxml2.h>
#include <filesystem>
#include <iostream>
#include <thread>

#pragma comment(lib, "tinyxml2.lib")

namespace xml2 = tinyxml2;

class Xml
{
public:
	auto Initialize(std::string&& sz_path = "") -> void
	{
		InitCwd();

		auto default_xml = Cwd() / "config.xml";

		if(!sz_path.empty()) {
			default_xml = sz_path;
		}

		m_sz_path_ = default_xml.generic_u8string();

		if(!std::filesystem::exists(m_sz_path_)) {
			std::cout << "Creating predifned configuration at " << m_sz_path_ << '\n';
			this->Save( m_sz_path_.c_str() );
		} else {
			std::cout << "Loading configuration at " << m_sz_path_ << '\n';
			this->Load( m_sz_path_.c_str() );
		}
	}

	auto Port(void) -> std::string&
	{
		return m_sz_port_;
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
	auto InitCwd(void) -> void
	{
		// get current working directory
		auto cwd = std::filesystem::current_path();

		// get process path
		char sz_exe_path[MAX_PATH];

		auto* const h_handle = GetModuleHandle(nullptr);
		
		GetModuleFileNameA(h_handle, sz_exe_path, MAX_PATH);

		m_fl_cwd_ = sz_exe_path;

		m_fl_cwd_.remove_filename();
	}

	auto Cwd(void) -> std::filesystem::path
	{
		return m_fl_cwd_;
	}

	auto Save(const char* sz_path) -> void
	{
		auto* comment = m_xml_doc_.NewComment("This file was created automatically.");
		m_xml_doc_.InsertEndChild(comment);

		auto* configuration = m_xml_doc_.NewElement("configuration");

		auto* port = m_xml_doc_.NewElement("connection");
		port->SetAttribute("port", "1337");
		configuration->InsertFirstChild(port);

		auto* prefix = m_xml_doc_.NewElement("echo-prefix");
		auto* e_prefix = m_xml_doc_.NewText("...");
		prefix->InsertEndChild(e_prefix);
		configuration->InsertEndChild(prefix);

		auto* suffix = m_xml_doc_.NewElement("echo-suffix");
		auto* e_suffix = m_xml_doc_.NewText("...");
		suffix->InsertEndChild(e_suffix);
		configuration->InsertEndChild(suffix);

		m_xml_doc_.InsertEndChild(configuration);

		if (m_xml_doc_.SaveFile(sz_path) != xml2::XML_SUCCESS) {
			std::cout << "Can't save predefined configuration" << '\n';
			std::exit(EXIT_FAILURE);
		}
	}

	auto Load(const char* sz_path) -> void
	{
		if (m_xml_doc_.LoadFile(sz_path) == xml2::XML_SUCCESS) {
			auto* root_element = m_xml_doc_.RootElement();

			auto* connection = root_element->FirstChildElement("connection");
			std::cout << "XML Port: " << connection->Attribute("port") << '\n';
			m_sz_port_ = connection->Attribute("port");

			auto* prefix = root_element->FirstChildElement("echo-prefix");
			std::cout << "XML Prefix: " << prefix->GetText() << '\n';
			m_sz_prefix_ = prefix->GetText();

			auto* suffix = root_element->FirstChildElement("echo-suffix");
			std::cout << "XML Suffix: " << suffix->GetText() << '\n';
			m_sz_suffix_ = suffix->GetText();
		}
	}
	
private:
	xml2::XMLDocument m_xml_doc_;
	std::filesystem::path m_fl_cwd_;
	std::string m_sz_path_;

	std::string m_sz_port_;
	std::string m_sz_prefix_;
	std::string m_sz_suffix_;
};

#endif // !XML_HPP
