// SPDX-License-Identifier: Apache-2.0

#ifndef _VIS_CONFIG_HPP
#define _VIS_CONFIG_HPP

#include <string>

class VisConfig
{
public:
        explicit VisConfig(const std::string &hostname,
			   const unsigned port,
			   const std::string &clientKey,
			   const std::string &clientCert,
			   const std::string &caCert,
			   const std::string &authToken,
			   bool verifyPeer = true);
        explicit VisConfig(const std::string &appname);
        ~VisConfig() {};

	std::string hostname() { return m_hostname; };
	unsigned port() { return m_port; };
	std::string clientKey() { return m_clientKey; };
	std::string clientCert() { return m_clientCert; };
	std::string caCert() { return m_caCert; };
	std::string authToken() { return m_authToken; };
	bool verifyPeer() { return m_verifyPeer; };
	bool valid() { return m_valid; };
	unsigned verbose() { return m_verbose; };

private:
	std::string m_hostname;
	unsigned m_port;
	std::string m_clientKey;
	std::string m_clientCert;
	std::string m_caCert;
	std::string m_authToken;
	bool m_verifyPeer;
	unsigned m_verbose;
	bool m_valid;
};

#endif // _VIS_CONFIG_HPP
