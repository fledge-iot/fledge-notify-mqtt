#ifndef _MQTT_H
#define _MQTT_H
#include <config_category.h>
#include <string>
#include <logger.h>
#include <mutex>


/**
 * A simple MQTT notification class that sends a message
 * via the MQTT broker when a notification triggers or clears
 */
class MQTT {
	public:
		MQTT(ConfigCategory *config);
		~MQTT();
		bool	notify(const std::string& notificationName, const std::string& triggerReason, const std::string& message);
		void	reconfigure(const std::string& newConfig);
	private:
		std::string	m_broker;
		std::string	m_topic;
		std::string	m_trigger;
		std::string	m_clear;
		std::mutex	m_mutex;
};
#endif
