// SPDX-License-Identifier: Apache-2.0

#include "monitor-service.hpp"
#include <iostream>
#include <algorithm>


MonitorService::MonitorService(const VisConfig &config, net::io_context& ioc, ssl::context& ctx) :
	VisSession(config, ioc, ctx),
	m_can_helper()
{
}

void MonitorService::handle_authorized_response(void)
{
	subscribe("Vehicle.TurboCharger.BoostLevel");
	// subscribe("Vehicle.TurboCharger.BoostPressure");
}

void MonitorService::handle_get_response(std::string &path, std::string &value, std::string &timestamp)
{
	// Placeholder since no gets are performed ATM
}

void MonitorService::handle_notification(std::string &path, std::string &value, std::string &timestamp)
{
	if (path == "Vehicle.TurboCharger.BoostLevel") {
		try {
			int level = std::stoi(value);
			if (level >= 0 && level < 100)
				set_level(level);
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} 
	/*
	else if (path == "Vehicle.TurboCharger.BoostPressure") {
		try {
			double pressure = std::stoi(value);
			if (pressure >= 0 && pressure < 5000.0)
				set_pressure(pressure);
				
		}
		catch (std::exception ex) {
			// ignore bad value
		}
	} */
	// else ignore
}

void MonitorService::set_level(uint8_t level)
{
	m_can_helper.set_level(level);
}

/*
void MonitorService::set_pressure(double pressure)
{
	m_can_helper.set_pressure(pressure);
}
*/
