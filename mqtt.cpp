/*
 * Fledge "MQTT" notification delivery plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch           
 */
#include "mqtt.h"
#include <logger.h>
#include <simple_https.h>
#include <rapidjson/document.h>
#include "MQTTClient.h"

#define TIMEOUT     10000L
#define CLIENTID    "FledgeNotification"

using namespace std;
using namespace rapidjson;


/**
 * Construct a MQTT notification plugin
 *
 * @param category	The configuration of the plugin
 */
MQTT::MQTT(ConfigCategory *category)
{
	if (category->itemExists("broker"))
		m_broker = category->getValue("broker");
	if (category->itemExists("topic"))
		m_topic = category->getValue("topic");
	if (category->itemExists("trigger_payload"))
		m_trigger = category->getValue("trigger_payload");
	if (category->itemExists("clear_payload"))
		m_clear = category->getValue("clear_payload");
}

/**
 * The destructure for the MQTT plugin
 */
MQTT::~MQTT()
{
}

/**
 * Send a notification via MQTT broker
 *
 * @param notificationName 	The name of this notification
 * @param triggerReason		Why the notification is being sent
 * @param message		The message to send
 */
bool MQTT::notify(const string& notificationName, const string& triggerReason, const string& message)
{
string 		payload = m_trigger;
MQTTClient	client;

	lock_guard<mutex> guard(m_mutex);

	// Parse the JSON that represents the reason data
	Document doc;
	doc.Parse(triggerReason.c_str());
	if (!doc.HasParseError() && doc.HasMember("reason"))
	{
		if (!strcmp(doc["reason"].GetString(), "cleared"))
			payload = m_clear;
	}


	// Connect to the MQTT broker
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	if ((rc = MQTTClient_create(&client, m_broker.c_str(), CLIENTID,
		MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
	{
		Logger::getLogger()->error("Failed to create client, return code %d\n", rc);
		return false;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		Logger::getLogger()->error("Failed to connect, return code %d\n", rc);
		return false;
	}

	// Construct the payload
	pubmsg.payload = (void *)payload.c_str();
	pubmsg.payloadlen = payload.length();
	pubmsg.qos = 1;
	pubmsg.retained = 0;

	// Publish the message
	if ((rc = MQTTClient_publishMessage(client, m_topic.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
	{
		Logger::getLogger()->error("Failed to publish message, return code %d\n", rc);
		return false;
	}

	// Wait for completion and disconnect
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
		Logger::getLogger()->error("Failed to disconnect, return code %d\n", rc);
	MQTTClient_destroy(&client);
	return true;
}

/**
 * Reconfigure the MQTT delivery plugin
 *
 * @param newConfig	The new configuration
 */
void MQTT::reconfigure(const string& newConfig)
{
	ConfigCategory category("new", newConfig);
	lock_guard<mutex> guard(m_mutex);
	m_broker = category.getValue("broker");
	m_topic = category.getValue("topic");
	m_trigger = category.getValue("trigger_payload");
	m_clear = category.getValue("clear_payload");
}
