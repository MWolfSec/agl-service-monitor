// SPDX-License-Identifier: Apache-2.0

#include "vis-config.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

namespace property_tree = boost::property_tree;
namespace filesystem = boost::filesystem;

#define DEFAULT_CLIENT_KEY_FILE  "/etc/kuksa-val/Client.key"
#define DEFAULT_CLIENT_CERT_FILE "/etc/kuksa-val/Client.pem"
#define DEFAULT_CA_CERT_FILE     "/etc/kuksa-val/CA.pem"


VisConfig::VisConfig(const std::string &hostname,
		     const unsigned port,
		     const std::string &clientKey,
		     const std::string &clientCert,
		     const std::string &caCert,
		     const std::string &authToken,
		     bool verifyPeer) :
	m_hostname(hostname),
	m_port(port),
	m_clientKey(clientKey),
	m_clientCert(clientCert),
	m_caCert(caCert),
	m_authToken(authToken),
	m_verifyPeer(verifyPeer),
	m_verbose(0),
	m_valid(true)
{
	// Potentially could do some certificate validation here...
}

VisConfig::VisConfig(const std::string &appname) :
	m_valid(false)
{
	std::string config("/etc/xdg/AGL/");
	config += appname;
	config += ".conf";
	char *home = getenv("XDG_CONFIG_HOME");
	if (home) {
		config = home;
		config += "/AGL/";
		config += appname;
		config += ".conf";
	}

	std::cout << "Vis Config - Using configuration " << config << std::endl;
	property_tree::ptree pt;
	try {
		std::cout << "Vis Config - Start read_ini " << std::endl;
		property_tree::ini_parser::read_ini(config, pt);
		std::cout << "Vis Config - Finished read_ini " << std::endl;
	}
	catch (std::exception &ex) {
		std::cerr << "Could not read " << config << std::endl;
		return;
	}
	const property_tree::ptree &settings =
		pt.get_child("vis-client", property_tree::ptree());

	m_hostname = settings.get("server", "localhost");
	std::stringstream ss;
	ss << m_hostname;
	ss >> std::quoted(m_hostname);
	if (m_hostname.empty()) {
		std::cerr << "Invalid server hostname" << std::endl;
		return;
	}

	m_port = settings.get("port", 8090);
	if (m_port == 0) {
		std::cerr << "Invalid server port" << std::endl;
		return;
	}

	// Default to disabling peer verification for now to be able
        // to use the default upstream KUKSA.val certificates for
	// testing.  Wrangling server and CA certificate generation
	// and management to be able to verify will require further
	// investigation.
	m_verifyPeer = settings.get("verify-server", false);

	std::string keyFileName = settings.get("key", DEFAULT_CLIENT_KEY_FILE);
	std::stringstream().swap(ss);
	ss << keyFileName;
	ss >> std::quoted(keyFileName);
	ss.str("");
	if (keyFileName.empty()) {
		std::cerr << "Invalid client key filename" << std::endl;
		return;
	}
	filesystem::load_string_file(keyFileName, m_clientKey);
	if (m_clientKey.empty()) {
		std::cerr << "Invalid client key file" << std::endl;
		return;
	}

	std::string certFileName = settings.get("certificate", DEFAULT_CLIENT_CERT_FILE);
	std::stringstream().swap(ss);
	ss << certFileName;
	ss >> std::quoted(certFileName);
	if (certFileName.empty()) {
		std::cerr << "Invalid client certificate filename" << std::endl;
		return;
	}
	filesystem::load_string_file(certFileName, m_clientCert);
	if (m_clientCert.empty()) {
		std::cerr << "Invalid client certificate file" << std::endl;
		return;
	}

	std::string caCertFileName = settings.get("ca-certificate", DEFAULT_CA_CERT_FILE);
	std::stringstream().swap(ss);
	ss << caCertFileName;
	ss >> std::quoted(caCertFileName);
	if (caCertFileName.empty()) {
		std::cerr << "Invalid CA certificate filename" << std::endl;
		return;
	}
	filesystem::load_string_file(caCertFileName, m_caCert);
	if (m_caCert.empty()) {
		std::cerr << "Invalid CA certificate file" << std::endl;
		return;
	}

	std::string authTokenFileName = settings.get("authorization", "");
	std::stringstream().swap(ss);
	ss << authTokenFileName;
	ss >> std::quoted(authTokenFileName);
	if (authTokenFileName.empty()) {
		std::cerr << "Invalid authorization token filename" << std::endl;
		return;
	}
	filesystem::load_string_file(authTokenFileName, m_authToken);
	if (m_authToken.empty()) {
		std::cerr << "Invalid authorization token file" << std::endl;
		return;
	}

	m_verbose = 1;
	std::string verbose = settings.get("verbose", "");
	std::stringstream().swap(ss);
	ss << verbose;
	ss >> std::quoted(verbose);
	if (!verbose.empty()) {
		if (verbose == "true" || verbose == "1")
			m_verbose = 1;
		if (verbose == "2")
			m_verbose = 2;
	}

	m_valid = true;
}
