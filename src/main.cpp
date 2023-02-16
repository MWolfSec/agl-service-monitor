// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <iomanip>
#include <boost/asio/signal_set.hpp>
#include <boost/bind.hpp>
#include "monitor-service.hpp"

using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

int main(int argc, char** argv)
{
	// The io_context is required for all I/O
	net::io_context ioc;

	// Register to stop I/O context on SIGINT and SIGTERM
	net::signal_set signals(ioc, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(&net::io_context::stop, &ioc));

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::tlsv12_client};

	// Launch the asynchronous operation
	VisConfig config("agl-service-monitor");
	std::make_shared<HvacService>(config, ioc, ctx)->run();

	// Ensure I/O context continues running even if there's no work
	work_guard_type work_guard(ioc.get_executor());

	// Run the I/O context
	ioc.run();

	return 0;
}
