// Config
#include "hardware_config.h"
#include "wifi_config.h"

// Libs
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>


// Sensors
DHT dht(DHT_PIN, DHT_MODEL);

// MQTT
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

// WiFi
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// State
float temp = 0.0f, hum = 0.0f, hic = 0.0f, moist = 0.0f;
unsigned long current = 0, prev_publish = 0, pump_start = 0;
bool pump_did_reset = true;


// Network handlers

void connectToWifi() 
{
	Serial.println("Connecting to Wi-Fi...");
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() 
{
	Serial.println("Connecting to MQTT...");
	mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) 
{
	Serial.println("Connected to Wi-Fi.");
	connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event)
{
	digitalWrite(LED_BUILTIN, HIGH);
	Serial.println("Disconnected from Wi-Fi.");

 	// Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
	mqttReconnectTimer.detach();
	wifiReconnectTimer.once(2, connectToWifi);
}

void subscribeMqttTopic(const char* topic)
{
	uint16_t packetIdSub = mqttClient.subscribe(topic, 2);
	Serial.printf("Subscribing at QoS 2, ID: %d\n", packetIdSub);
}

void onMqttConnect(bool sessionPresent)
{
	digitalWrite(LED_BUILTIN, LOW);
	Serial.printf("Connected to MQTT. Session present: %d\n", sessionPresent);

	// Subscribe to topics
	subscribeMqttTopic(MQTT_PUB_PUMP);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) 
{
	digitalWrite(LED_BUILTIN, HIGH);
	Serial.println("Disconnected from MQTT.");
	if (WiFi.isConnected()) mqttReconnectTimer.once(2, connectToMqtt);
}

void onMqttPublish(uint16_t packetId) 
{
	Serial.printf("Publish acknowledged, ID: %d\n", packetId);
}

void publishMqttMessage(const char* topic, const char* payload, uint8_t qos = 1)
{
	uint16_t id = mqttClient.publish(topic, qos, true, payload);                            
	Serial.printf("Publishing on topic %s at QoS %d, ID: %i, message: %s\n", topic, qos, id, payload);

	// Cooldown a bit to not overload the stack
	delay(MQTT_MSG_COOLDOWN);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) 
{
	Serial.printf("Subscribe acknowledged. ID: %d, qos: %d\n", packetId, qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
	Serial.printf("Unsubscribe acknowledged. ID: %d\n", packetId);
}


// Core logic

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	// Handle pump state change
	if(!strcmp(topic, MQTT_PUB_PUMP) && atoi(payload) == 1) 
	{
		pump_did_reset = false;
		pump_start = millis();
	}
	
  	Serial.printf("Publish received. topic: %s, payload: %s, qos: %d, dup: %d, retain %d, len: %d, index: %d, total: %d\n",
  		topic, payload, properties.qos, properties.dup, properties.retain, len, index, total);
}

void setup() 
{
	Serial.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// Init hardware
	dht.begin();
	pinMode(PUMP_PIN, OUTPUT);
	digitalWrite(PUMP_PIN, LOW);

	// Init network
	wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
	wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.onPublish(onMqttPublish);
	mqttClient.onMessage(onMqttMessage);
	mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
	mqttClient.setServer(MQTT_HOST, MQTT_PORT);
#	if MQTT_NEEDS_SSL
#		if !defined(ASYNC_TCP_SSL_ENABLED) || ASYNC_TCP_SSL_ENABLED == 0
#			error MQTT_NEEDS_SSL is defined, but project is not being compiled with ASYNC_TCP_SSL_ENABLED=1
#		else
			mqttClient.setSecure(true);
#		endif
#	endif 
#	if MQTT_NEEDS_AUTH
		mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
#	endif
	connectToWifi();
}

void loop()
{
	unsigned long current = millis();

	// Control pump
	int pump_state = pump_start != 0 && (current - pump_start < PUMP_DURATION)? HIGH:LOW;
	digitalWrite(PUMP_PIN, pump_state);
	if(pump_state == LOW && !pump_did_reset)
	{
		publishMqttMessage(MQTT_PUB_PUMP, "0", 2);
		pump_did_reset = true;
	}

	// Ensure publishing interval is adhered to
	if (current - prev_publish >= MQTT_PUB_INTERVAL) 
	{
		prev_publish = current;

		// Read DHT sensor
		hum = dht.readHumidity();
		temp = dht.readTemperature(false);
		if (isnan(hum) || isnan(temp)) 
		{
			Serial.println("Failed to read from DHT sensor!");
			return;
		}
		hic = dht.computeHeatIndex(temp, hum, false);
		
		// Read soil sensor
		moist = (float)map(analogRead(SOIL_PIN), 1024, 500, 0, 100);

		// Publish DHT messages
		publishMqttMessage(MQTT_PUB_HUM, String(hum).c_str());
		publishMqttMessage(MQTT_PUB_TEMP, String(temp).c_str());
		publishMqttMessage(MQTT_PUB_IDX, String(hic).c_str());
		publishMqttMessage(MQTT_PUB_SOIL, String(moist).c_str());
	}
}