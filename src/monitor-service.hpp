// SPDX-License-Identifier: Apache-2.0

#ifndef _MONITOR_SERVICE_HPP
#define _MONITOR_SERVICE_HPP

#include "vis-session.hpp"
#include "monitor-can-helper.hpp"

class MonitorService : public VisSession
{
public:
	MonitorService(const VisConfig &config, net::io_context& ioc, ssl::context& ctx);

protected:
	virtual void handle_authorized_response(void) override;

	virtual void handle_get_response(std::string &path, std::string &value, std::string &timestamp) override;

	virtual void handle_notification(std::string &path, std::string &value, std::string &timestamp) override;

private:
	MonitorCanHelper m_can_helper;

	void set_level(uint8_t level);

	void set_pressure(double pressure);

};

#endif // _AVL_SERVICE_HPP
