// DHT SENSOR
#define NUM_DHTS 2
#define DHT_DEFS { DHT(12, DHT11), DHT(10, DHT11) }

// SOIL SENSOR
#define NUM_SOILS 2
#define SOIL_INDICES { 0, 1 }
#define SOIL_MIN { 380, 530 }
#define SOIL_MAX { 1100, 1100 }

// PUMPS
#define NUM_PUMPS 2
#define PUMP_PINS { 14, 13 }
#define PUMP_DURATION 2000

// FAN
#define FAN_PIN 15