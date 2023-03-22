// SPDX-License-Identifier: Apache-2.0

#include "monitor-can-helper.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace property_tree = boost::property_tree;

MonitorCanHelper::MonitorCanHelper() :
	m_level(30),
	// m_pressure(800.0),
	m_port("can0"),
	m_config_valid(false),
	m_active(false),
	m_verbose(1)
{
	read_config();

	can_open();
}

MonitorCanHelper::~MonitorCanHelper()
{
	can_close();
}

void MonitorCanHelper::read_config()
{
	// Using a separate configuration file now, it may make sense
	// to revisit this if a workable scheme to handle overriding
	// values for the full demo setup can be come up with.
	std::string config("/etc/xdg/AGL/agl-service-monitor-can.conf");
	char *home = getenv("XDG_CONFIG_HOME");
	if (home) {
		config = home;
		config += "/AGL/agl-service-monitor.conf";
	}


	std::cout << "Using configuration " << config << std::endl;
	property_tree::ptree pt;
	try {
		property_tree::ini_parser::read_ini(config, pt);
	}
	catch (std::exception &ex) {
		// Continue with defaults if file missing/broken
		std::cerr << "Could not read " << config << std::endl;
		m_config_valid = true;
		return;
	}
	const property_tree::ptree &settings =
		pt.get_child("can", property_tree::ptree());

	m_port = settings.get("port", "can0");
	std::stringstream ss;
	ss << m_port;
	ss >> std::quoted(m_port);
	if (m_port.empty()) {
		std::cerr << "Invalid CAN port path" << std::endl;
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

	m_config_valid = true;
}

void MonitorCanHelper::can_open()
{
	if (!m_config_valid)
		return;

	if (m_verbose > 1)
		std::cout << "MonitorCanHelper::MonitorCanHelper: using port " << m_port << std::endl;

	// Open raw CAN socket
	m_can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (m_can_socket < 0) {
		return;
	}

	// Look up port address
	struct ifreq ifr;
	strcpy(ifr.ifr_name, m_port.c_str());
	if (ioctl(m_can_socket, SIOCGIFINDEX, &ifr) < 0) {
		close(m_can_socket);
		return;
	}

	m_can_addr.can_family = AF_CAN;
	m_can_addr.can_ifindex = ifr.ifr_ifindex;
	if (bind(m_can_socket, (struct sockaddr*) &m_can_addr, sizeof(m_can_addr)) < 0) {
		close(m_can_socket);
		return;
	}

	m_active = true;
	if (m_verbose > 1)
		std::cout << "MonitorCanHelper::MonitorCanHelper: opened " << m_port << std::endl;
}

void MonitorCanHelper::can_close()
{
	if (m_active)
		close(m_can_socket);
}

void MonitorCanHelper::set_level(uint8_t level)
{
	m_level = level;
	can_update();
}

/*
void MonitorCanHelper::set_pressure(double pressure)
{
	m_pressure = pressure;
	can_update();
}
*/

void MonitorCanHelper::can_update()
{
	if (!m_active)
		return;

	struct can_frame frame;
	frame.can_id = 0x201;
	frame.can_dlc = 8;
	frame.data[0] = 0;
	frame.data[1] = convert_level(m_level);
	frame.data[2] = 0;
	frame.data[3] = 0;
	frame.data[4] = 0; //convert_pressure(m_pressure);
	frame.data[5] = 0;
	frame.data[6] = 0;
	frame.data[7] = 0;

	auto written = sendto(m_can_socket,
			      &frame,
			      sizeof(struct can_frame),
			      0,
			      (struct sockaddr*) &m_can_addr,
			      sizeof(m_can_addr));
	if (written < 0) {
		std::cerr << "Write to " << m_port << " failed!" << std::endl;
		close(m_can_socket);
		m_active = false;
	}
}


uint8_t MonitorCanHelper::convert_level(uint8_t value) {
	int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
		if (result < 0x10)
			result = 0x10;
		if (result > 0xF0)
			result = 0xF0;

		return (uint8_t) result;
}

/*
uint8_t  MonitorCanHelper::convert_pressure(double value) {
	int result = ((0xF0 - 0x10) / 15) * (((int)value/100) - 15) + 0x10;
	if (result < 0x10)
		result = 0x10;
	if (result > 0xF0)
		result = 0xF0;

	return (uint8_t) result;
}
*/

