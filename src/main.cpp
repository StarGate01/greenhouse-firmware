// Config
#include "hardware_config.h"
#include "wifi_config.h"

// Libs
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <Adafruit_VEML6070.h>

// Sensors
DHT dhts[NUM_DHTS] = DHT_DEFS;
BH1750 light;
Adafruit_VEML6070 uvlight;

// Network
#if MQTT_NEEDS_ENCRYPTION
    WiFiClientSecure wifi_client;
#else
    WiFiClient wifi_client;
#endif
PubSubClient mqtt_client(wifi_client);
char mqtt_payload_buffer[16];
char mqtt_topic_buffer[32];

// State
struct sdata_t
{
    float temp[NUM_DHTS] = {0};
    float hum[NUM_DHTS] = {0};
    float hic[NUM_DHTS] = {0};
    float moist[NUM_SOILS] = {0};
    float lux = 0.0f;
    float uv = 0.0f;
    unsigned int uvi = 0;
} sensor_data;

struct pump_t
{
    unsigned int pin = 0;
    unsigned long start = 0;
    bool did_reset = true;
} pumps[2];

unsigned long current = 0, prev_publish = 0;


// Core logic

void mqtt_publish_buffer(const char* tbuffer, const char* pbuffer)
{
    bool res = mqtt_client.publish(tbuffer, pbuffer);
	Serial.printf("Published on topic %s, value: %s, ok: %d\n", tbuffer, pbuffer, res?1:0);
    delay(MQTT_MSG_COOLDOWN);
}

void mqtt_publish(const char* topic, float payload)
{
    sprintf(mqtt_payload_buffer, "%.2f", payload);
	mqtt_publish_buffer(topic, mqtt_payload_buffer);
}

void mqtt_publish(const char* topic, unsigned int payload)
{
    sprintf(mqtt_payload_buffer, "%d", payload);
	mqtt_publish_buffer(topic, mqtt_payload_buffer);
}

void mqtt_publish_sensors(const sdata_t* data)
{
    for(int i=0; i<NUM_DHTS; i++)
    {
        sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_HUM, i);
        mqtt_publish(mqtt_topic_buffer, data->hum[i]);
        sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_TEMP, i);
        mqtt_publish(mqtt_topic_buffer, data->temp[i]);
        sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_IDX, i);
        mqtt_publish(mqtt_topic_buffer, data->hic[i]);
    }
    for(int i=0; i<NUM_SOILS; i++)
    {
        sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_SOIL, i);
        mqtt_publish(mqtt_topic_buffer, data->moist[i]);
    }
    mqtt_publish(MQTT_PUB_LUX, data->lux);
    mqtt_publish(MQTT_PUB_UV, data->uv);
    mqtt_publish(MQTT_PUB_UVI, data->uvi);
}

void mqtt_reconnect() 
{
    while (!mqtt_client.connected()) 
    {
        Serial.print("Connecting to MQTT broker... ");
#       if MQTT_NEEDS_AUTH
            if (mqtt_client.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD))
#       else
            if (mqtt_client.connect(MQTT_NAME))
#       endif
        {
            Serial.println("MQTT connected");
            digitalWrite(LED_BUILTIN, LOW);
            delay(200);
            digitalWrite(LED_BUILTIN, HIGH);

            // Subscribe to pumps
            for(int i=0; i<NUM_PUMPS; i++)
            {
                sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_PUMP, i);
                bool res = mqtt_client.subscribe(mqtt_topic_buffer, 1);
                Serial.printf("Subscribed to: %s, ok: %d\n", mqtt_topic_buffer, res?1:0);
                mqtt_publish(mqtt_topic_buffer, 0u);
            }
        } 
        else 
        {
            Serial.printf("MQTT connection failed, rc=%d, retrying in 3s...\n", mqtt_client.state());
            delay(3000);
        }
    }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    Serial.printf("Got subscription callback at %s: %.*s\n", topic, length, payload);

    // Handle pump state change
    size_t pump_base_len = strlen(MQTT_PUB_PUMP);
	if(!strncmp(topic, MQTT_PUB_PUMP, pump_base_len)) 
	{
        int index = atoi(topic + pump_base_len + 1);
        if(length > 0 && payload[0] == '1')
        {
            pumps[index].did_reset = false;
            pumps[index].start = millis();
            Serial.printf("Got pump command, index: %d\n", index);
        }
	}
}

void setup()
{
    // Init debugging
    Serial.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// Init hardware
	Wire.begin();
    for(int i=0; i<NUM_DHTS; i++)
    {
	    dhts[i].begin();
    }
	light.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
	uvlight.begin(VEML6070_1_T); 
    int pump_pins[] = PUMP_PINS;
    for(int i=0; i<NUM_PUMPS; i++)
    {
        pinMode(pump_pins[i], OUTPUT);
        digitalWrite(pump_pins[i], LOW);
        pumps[i].pin = pump_pins[i];
    }
    delay(10);

    // Init network
    Serial.println("Connecting to WiFi... ");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) yield();
    Serial.printf("WiFi connected, IP address: %s\n", WiFi.localIP().toString().c_str());
#   if MQTT_NEEDS_ENCRYPTION
        wifi_client.setFingerprint(MQTT_FINGERPRINT);
#   endif
    mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_client.setCallback(mqtt_callback);
}



void loop() 
{
    // Ensure MQTT connection
    if (!mqtt_client.connected()) 
    {
        mqtt_reconnect();
    }

    current = millis();

	// Control pumps
    for(int i=0; i<NUM_PUMPS; i++)
    {
        int pump_state = pumps[i].start != 0 && (current - pumps[i].start < PUMP_DURATION)? HIGH:LOW;
        digitalWrite(pumps[i].pin, pump_state);
        if(pump_state == LOW && !pumps[i].did_reset)
        {
            sprintf(mqtt_topic_buffer, "%s/%d", MQTT_PUB_PUMP, i);
            mqtt_publish(mqtt_topic_buffer, 0u);
            pumps[i].did_reset = true;
        }
    }

	// Ensure publishing interval is adhered to
	if (current - prev_publish >= MQTT_PUB_INTERVAL) 
	{
		prev_publish = current;

		// Read DHT sensors
        for(int i=0; i<NUM_DHTS; i++)
        {
            sensor_data.hum[i] = dhts[i].readHumidity();
            sensor_data.temp[i] = dhts[i].readTemperature(false);
            if (!isnan(sensor_data.hum[i]) && !isnan(sensor_data.temp[i])) 
            {
                sensor_data.hic[i] = dhts[i].computeHeatIndex(sensor_data.temp[i], sensor_data.hum[i], false);
            }
        }
		
		// Read soil sensors
        int soil_pins[] = SOIL_PINS;
        for(int i=0; i<NUM_SOILS; i++)
        {
		    sensor_data.moist[i] = (float)map(analogRead(soil_pins[i]), 1024, 500, 0, 100);
        }

		// Read lux sensor
		while (!light.measurementReady(true)) yield();
		sensor_data.lux = light.readLightLevel();
		light.configure(BH1750::ONE_TIME_HIGH_RES_MODE);

		// Read uv sensor
		sensor_data.uv = uvlight.readUV();
		sensor_data.uvi = 0.4 * (sensor_data.uv * 5.625) / 1000;

		// Publish DHT messages
        mqtt_publish_sensors(&sensor_data);
	}

    mqtt_client.loop();
}