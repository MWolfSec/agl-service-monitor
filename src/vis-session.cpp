// SPDX-License-Identifier: Apache-2.0

#include "vis-session.hpp"
#include <iostream>
#include <sstream>
#include <thread>


// Logging helper
static void log_error(beast::error_code error, char const* what)
{
	std::cerr << what << " error: " << error.message() << std::endl;
}


// Resolver and socket require an io_context
VisSession::VisSession(const VisConfig &config, net::io_context& ioc, ssl::context& ctx) :
	m_config(config),
	m_resolver(net::make_strand(ioc)),
	m_ws(net::make_strand(ioc), ctx)
{
}

// Start the asynchronous operation
void VisSession::run()
{
	if (!m_config.valid()) {
		return;
	}

	// Start by resolving hostname
	m_resolver.async_resolve(m_config.hostname(),
				 std::to_string(m_config.port()),
				 beast::bind_front_handler(&VisSession::on_resolve,
							   shared_from_this()));
}

void VisSession::on_resolve(beast::error_code error,
			    tcp::resolver::results_type results)
{
	if(error) {
		log_error(error, "resolve");
		return;
	}

	// Set a timeout on the connect operation
	beast::get_lowest_layer(m_ws).expires_after(std::chrono::seconds(30));

	// Connect to resolved address
	if (m_config.verbose())
		std::cout << "Connecting" << std::endl;
	m_results = results;
	connect();
}

void VisSession::connect()
{
	beast::get_lowest_layer(m_ws).async_connect(m_results,
						    beast::bind_front_handler(&VisSession::on_connect,
									      shared_from_this()));
}

void VisSession::on_connect(beast::error_code error,
			    tcp::resolver::results_type::endpoint_type endpoint)
{
	if(error) {
		// The server can take a while to be ready to accept connections,
		// so keep retrying until we hit the timeout.
		if (error == net::error::timed_out) {
			log_error(error, "connect");
			return;
		}

		// Delay 500 ms before retrying
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		if (m_config.verbose())
			std::cout << "Connecting" << std::endl;
		
		connect();
		return;
	}

	if (m_config.verbose())
		std::cout << "Connected" << std::endl;

	// Set handshake timeout
	beast::get_lowest_layer(m_ws).expires_after(std::chrono::seconds(30));

	// Set SNI Hostname (many hosts need this to handshake successfully)
	if(!SSL_set_tlsext_host_name(m_ws.next_layer().native_handle(),
				     m_config.hostname().c_str()))
	{
		error = beast::error_code(static_cast<int>(::ERR_get_error()),
					  net::error::get_ssl_category());
		log_error(error, "connect");
		return;
	}

	// Update the hostname. This will provide the value of the
	// Host HTTP header during the WebSocket handshake.
	// See https://tools.ietf.org/html/rfc7230#section-5.4
	m_hostname = m_config.hostname() + ':' + std::to_string(endpoint.port());

	if (m_config.verbose())
		std::cout << "Negotiating SSL handshake" << std::endl;

	// Perform the SSL handshake
	m_ws.next_layer().async_handshake(ssl::stream_base::client,
					  beast::bind_front_handler(&VisSession::on_ssl_handshake,
								    shared_from_this()));
}

void VisSession::on_ssl_handshake(beast::error_code error)
{
	if(error) {
		log_error(error, "SSL handshake");
		return;
	}

	// Turn off the timeout on the tcp_stream, because
	// the websocket stream has its own timeout system.
	beast::get_lowest_layer(m_ws).expires_never();
	
	// NOTE: Explicitly not setting websocket stream timeout here,
	//       as the client is long-running.

	if (m_config.verbose())
		std::cout << "Negotiating WSS handshake" << std::endl;

	// Perform handshake
	m_ws.async_handshake(m_hostname,
			     "/",
			     beast::bind_front_handler(&VisSession::on_handshake,
						       shared_from_this()));
}

void VisSession::on_handshake(beast::error_code error)
{
	if(error) {
		log_error(error, "WSS handshake");
		return;
	}

	if (m_config.verbose())
		std::cout << "Authorizing" << std::endl;

	// Authorize
	json req;
	req["requestId"] = std::to_string(m_requestid++);
	req["action"]= "authorize";
	req["tokens"] = m_config.authToken();
	
	m_ws.async_write(net::buffer(req.dump(4)),
			 beast::bind_front_handler(&VisSession::on_authorize,
						   shared_from_this()));
}

void VisSession::on_authorize(beast::error_code error, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if(error) {
		log_error(error, "authorize");
		return;
	}

	// Read response
	m_ws.async_read(m_buffer,
			beast::bind_front_handler(&VisSession::on_read,
						  shared_from_this()));
}

// NOTE: Placeholder for now
void VisSession::on_write(beast::error_code error, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if(error) {
		log_error(error, "write");
		return;
	}

	// Do nothing...
}

void VisSession::on_read(beast::error_code error, std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if(error) {
		log_error(error, "read");
		return;
	}

	// Handle message
	std::string s = beast::buffers_to_string(m_buffer.data());
	json response = json::parse(s, nullptr, false);
	if (!response.is_discarded()) {
		handle_message(response);
	} else {
		std::cerr << "json::parse failed? got " << s << std::endl;
	}
	m_buffer.consume(m_buffer.size());

	// Read next message
	m_ws.async_read(m_buffer,
			beast::bind_front_handler(&VisSession::on_read,
						  shared_from_this()));
}

void VisSession::get(const std::string &path)
{
	if (!m_config.valid()) {
		return;
	}

	json req;
	req["requestId"] = std::to_string(m_requestid++);
	req["action"] = "get";
	req["path"] = path;
	req["tokens"] = m_config.authToken();

	m_ws.write(net::buffer(req.dump(4)));
}

void VisSession::set(const std::string &path, const std::string &value)
{
	if (!m_config.valid()) {
		return;
	}

	json req;
	req["requestId"] = std::to_string(m_requestid++);
	req["action"] = "set";
	req["path"] = path;
	req["value"] = value;
	req["tokens"] = m_config.authToken();
	
	m_ws.write(net::buffer(req.dump(4)));
}

void VisSession::subscribe(const std::string &path)
{
	if (!m_config.valid()) {
		return;
	}

	json req;
	req["requestId"] = std::to_string(m_requestid++);
	req["action"] = "subscribe";
	req["path"] = path;
	req["tokens"] = m_config.authToken();
	
	m_ws.write(net::buffer(req.dump(4)));
}

bool VisSession::parseData(const json &message, std::string &path, std::string &value, std::string &timestamp)
{
	if (message.contains("error")) {
		std::string error = message["error"];
		return false;
	}

	if (!(message.contains("data") && message["data"].is_object())) {
		std::cerr << "Malformed message (data missing)" << std::endl;
		return false;
	}
	auto data = message["data"];
	if (!(data.contains("path") && data["path"].is_string())) {
		std::cerr << "Malformed message (path missing)" << std::endl;
		return false;
	}
	path = data["path"];
	// Convert '/' to '.' in paths to ensure consistency for clients
	std::replace(path.begin(), path.end(), '/', '.');

	if (!(data.contains("dp") && data["dp"].is_object())) {
		std::cerr << "Malformed message (datapoint missing)" << std::endl;
		return false;
	}
	auto dp = data["dp"];
	if (!dp.contains("value")) {
		std::cerr << "Malformed message (value missing)" << std::endl;
		return false;
	} else if (dp["value"].is_string()) {
		value = dp["value"];
	} else if (dp["value"].is_number_float()) {
		double num = dp["value"];
		value = std::to_string(num);
	} else if (dp["value"].is_number_unsigned()) {
		unsigned int num = dp["value"];
		value = std::to_string(num);
	} else if (dp["value"].is_number_integer()) {
		int num = dp["value"];
		value = std::to_string(num);
	} else if (dp["value"].is_boolean()) {
		value = dp["value"] ? "true" : "false";
	} else {
		std::cerr << "Malformed message (unsupported value type)" << std::endl;
		return false;
	}

	if (!(dp.contains("ts") && dp["ts"].is_string())) {
		std::cerr << "Malformed message (timestamp missing)" << std::endl;
		return false;
	}
	timestamp = dp["ts"];

	return true;
}

void VisSession::handle_message(const json &message)
{
	if (m_config.verbose() > 1)
		std::cout << "VisSession::handle_message: enter, message = " << to_string(message) << std::endl;

	if (!message.contains("action")) {
		std::cerr << "Received unknown message (no action), discarding" << std::endl;
		return;
	}
	
	std::string action = message["action"];
	if (action == "authorize") {
		if (message.contains("error")) {
			std::string error = "unknown";
			if (message["error"].is_object() && message["error"].contains("message"))
				error = message["error"]["message"];
			std::cerr << "VIS authorization failed: " << error << std::endl;
		} else {
			if (m_config.verbose() > 1)
				std::cout << "authorized" << std::endl;

			handle_authorized_response();
		}
	} else if (action == "subscribe") {
		if (message.contains("error")) {
			std::string error = "unknown";
			if (message["error"].is_object() && message["error"].contains("message"))
				error = message["error"]["message"];
			std::cerr << "VIS subscription failed: " << error << std::endl;
		}
	} else if (action == "get") {
		if (message.contains("error")) {
			std::string error = "unknown";
			if (message["error"].is_object() && message["error"].contains("message"))
				error = message["error"]["message"];
			std::cerr << "VIS get failed: " << error << std::endl;
		} else {
			std::string path, value, ts;
			if (parseData(message, path, value, ts)) {
				if (m_config.verbose() > 1)
					std::cout << "VisSession::handle_message: got response " << path << " = " << value << std::endl;

				handle_get_response(path, value, ts);
			}
		}
	} else if (action == "set") {
		if (message.contains("error")) {
			std::string error = "unknown";
			if (message["error"].is_object() && message["error"].contains("message"))
				error = message["error"]["message"];
			std::cerr << "VIS set failed: " << error;
		}
	} else if (action == "subscription") {
		std::string path, value, ts;
		if (parseData(message, path, value, ts)) {
			if (m_config.verbose() > 1)
				std::cout << "VisSession::handle_message: got notification " << path << " = " << value << std::endl;

			handle_notification(path, value, ts);
		}
	} else {
		std::cerr << "unhandled VIS response of type: " << action;
	}

	if (m_config.verbose() > 1)
		std::cout << "VisSession::handle_message: exit" << std::endl;
}

