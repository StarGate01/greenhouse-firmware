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

void onMqttConnect(bool sessionPresent)
{
	digitalWrite(LED_BUILTIN, LOW);
	Serial.printf("Connected to MQTT. Session present: %d\n", sessionPresent);
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

void publishMqttMessage(const char* topic, const char* payload)
{
	uint16_t id = mqttClient.publish(topic, 1, true, payload);                            
	Serial.printf("Publishing on topic %s at QoS 1, ID: %i, message: %s\n", topic, id, payload);

	// Cooldown a bit to not overload the stack
	delay(MQTT_MSG_COOLDOWN);
}


// Core logic

float temp, hum, hic;
unsigned long previousMillis = 0;

void setup() 
{
	Serial.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// Init sensors
	dht.begin();

	// Init network
	wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
	wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

	mqttClient.onConnect(onMqttConnect);
	mqttClient.onDisconnect(onMqttDisconnect);
	mqttClient.onPublish(onMqttPublish);
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
	// Ensure publishing interval is adhered to
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= MQTT_PUB_INTERVAL) 
	{
		previousMillis = currentMillis;

		// Read DHT sensor
		hum = dht.readHumidity();
		temp = dht.readTemperature(false);
		if (isnan(hum) || isnan(temp)) 
		{
			Serial.println("Failed to read from DHT sensor!");
			return;
		}
		hic = dht.computeHeatIndex(temp, hum, false);
		
		// Publish DHT messages, slow down a bit to now o
		publishMqttMessage(MQTT_PUB_HUM, String(hum).c_str());
		publishMqttMessage(MQTT_PUB_TEMP, String(temp).c_str());
		publishMqttMessage(MQTT_PUB_IDX, String(hic).c_str());
	}
}