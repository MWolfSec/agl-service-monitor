// SPDX-License-Identifier: Apache-2.0

#include "monitor-service.hpp"
#include <iostream>
#include <algorithm>


MonitorService::MonitorService(const VisConfig &config, net::io_context& ioc, ssl::context& ctx) :
	VisSession(config, ioc, ctx),
	m_can_helper(),
	m_led_helper()
{
}

void MonitorService::handle_authorized_response(void)
{
	subscribe("Vehicle.Speed");
	subscribe("Vehicle.Powertrain.CombustionEngine.Speed");
}

void MonitorService::handle_get_response(std::string &path, std::string &value, std::string &timestamp)
{
	// Placeholder since no gets are performed ATM
}

void MonitorService::handle_notification(std::string &path, std::string &value, std::string &timestamp)
{
	if (path == "Vehicle.Speed") {
		try {
			int temp = std::stoi(value);
			if (temp >= 0 && temp < 256)
				set_right_temperature(temp);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} else if (path == "Vehicle.Powertrain.CombustionEngine.Speed") {
		try {
			int speed = std::stoi(value);
			if (speed >= 0 && speed < 256)
				set_fan_speed(speed);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} 
	// else ignore
}

void MonitorService::set_speed(uint8_t speed)
{
	m_can_helper.set_speed(speed);
}

void MonitorService::set_rpm(uint8_t rpm)
{
	m_can_helper.set_rpm(rpm);
}

