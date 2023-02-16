// SPDX-License-Identifier: Apache-2.0

#ifndef _VIS_SESSION_HPP
#define _VIS_SESSION_HPP

#include "vis-config.hpp"
#include <atomic>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


class VisSession : public std::enable_shared_from_this<VisSession>
{
	//net::io_context m_ioc;
	tcp::resolver m_resolver;
	tcp::resolver::results_type m_results;
	std::string m_hostname;
	websocket::stream<beast::ssl_stream<beast::tcp_stream>> m_ws;
	beast::flat_buffer m_buffer;

public:
	// Resolver and socket require an io_context
	explicit VisSession(const VisConfig &config, net::io_context& ioc, ssl::context& ctx);

	// Start the asynchronous operation
	void run();

protected:
	VisConfig m_config;
	std::atomic_uint m_requestid;

	void on_resolve(beast::error_code error, tcp::resolver::results_type results);

	void connect();

	void on_connect(beast::error_code error, tcp::resolver::results_type::endpoint_type endpoint);

	void on_ssl_handshake(beast::error_code error);

	void on_handshake(beast::error_code error);

	void on_authorize(beast::error_code error, std::size_t bytes_transferred);

	void on_write(beast::error_code error, std::size_t bytes_transferred);

	void on_read(beast::error_code error, std::size_t bytes_transferred);

	void get(const std::string &path);

	void set(const std::string &path, const std::string &value);

	void subscribe(const std::string &path);

	void handle_message(const json &message);

	bool parseData(const json &message, std::string &path, std::string &value, std::string &timestamp);

	virtual void handle_authorized_response(void) = 0;

	virtual void handle_get_response(std::string &path, std::string &value, std::string &timestamp) = 0;

	virtual void handle_notification(std::string &path, std::string &value, std::string &timestamp) = 0;

};

#endif // _VIS_SESSION_HPP
