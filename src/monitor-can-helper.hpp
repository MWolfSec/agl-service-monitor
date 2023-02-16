// SPDX-License-Identifier: Apache-2.0

#ifndef _AVL_CAN_HELPER_HPP
#define _AVL_CAN_HELPER_HPP

#include <string>
#include <linux/can.h>

class MonitorCanHelper
{
public:
	MonitorCanHelper();

	~MonitorCanHelper();

	void set_speed(uint8_t speed);

	void set_rpm(uint8_t rpm);

private:
	uint8_t convert_temp(uint8_t value) {
		int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
		if (result < 0x10)
			result = 0x10;
		if (result > 0xF0)
			result = 0xF0;

		return (uint8_t) result;
	}

	uint8_t convert_speed(uint8_t value) {
		int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
		if (result < 0x10)
			result = 0x10;
		if (result > 0xF0)
			result = 0xF0;

		return (uint8_t) result;
	}

	uint8_t convert_rpm(uint8_t value) {
		int result = ((0xF0 - 0x10) / 15) * (value - 15) + 0x10;
		if (result < 0x10)
			result = 0x10;
		if (result > 0xF0)
			result = 0xF0;

		return (uint8_t) result;
	}

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

	uint8_t m_temp_left;
	uint8_t m_temp_right;
	uint8_t m_fan_speed;
};

#endif // _AVL_CAN_HELPER_HPP
