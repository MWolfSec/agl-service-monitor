// SPDX-License-Identifier: Apache-2.0

#ifndef _MONITOR_CAN_HELPER_HPP
#define _MONITOR_CAN_HELPER_HPP

#include <string>
#include <linux/can.h>

class MonitorCanHelper
{
public:
	MonitorCanHelper();

	~MonitorCanHelper();

	void set_level(uint8_t level);

	// void set_pressure(double pressure);

private:

	uint8_t convert_level(uint8_t value);
	
	// uint8_t  convert_pressure(double value);

	void read_config();

	void can_open();

	void can_close();

	void can_update();

	std::string m_port;
	unsigned m_verbose;
	bool m_config_valid;
	bool m_active;
	int m_can_socket;
	struct sockaddr_can m_can_addr;

	uint8_t m_level;
	// double m_pressure;
};

#endif // _MONITOR_CAN_HELPER_HPP
