#include <UIPEthernet.h>
#include <UIPClient.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_DATA_REQUEST "sensor/dataRequest"
#define MQTT_TOPIC_DATA "sensor/data"

#define MQTT_BROKER_HOST "192.168.0.123"
#define MQTT_BROKER_PORT 1883

#define MQTT_ID "arduino"

#define COMPRESSOR_RELAY_PIN 2
#define PRESSURE_SENSOR_PIN 0
#define MOISTURE_SENSOR_PIN 1

byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x31 };
//IPAddress ip(192,168,0,179);

EthernetClient ethernet_client;
PubSubClient mqtt_client;


// Get pressure in pascals
float get_pressure() {
    // Turn on the compressor for 5 seconds
    digitalWrite(COMPRESSOR_RELAY_PIN, HIGH);
    delay(5000);
    digitalWrite(COMPRESSOR_RELAY_PIN, LOW);

    // Let the pressure stabilize
    delay(2000);

    float pressure_sensor_value = analogRead(PRESSURE_SENSOR_PIN);
    float pressure = ((pressure_sensor_value/1024.0)-0.0312)/0.000009;

    return pressure;
}

// Get water level in meters
float get_water_level() {
    float kPa = get_pressure() / 1000;
    float water_level = kPa / 10;

    return water_level;
}

// Get soil moisture level in %
float get_moisture_level() {
    return (analogRead(MOISTURE_SENSOR_PIN) / 1024.0f) * 100;
}

void mqtt_handle_data_request(byte *payload, unsigned int length) {
    float water_level = get_water_level();
    float moisture_level = get_moisture_level();

    // Build the JSON string
    // {
    //   'water_level': xyz
    //   'moisture_level': xyz
    // }
    String json_string = String("{'water_level':" + String(water_level, 1) + ",'moisture_level':" + String(moisture_level, 1) + "}");

    Serial.println(json_string);

    mqtt_client.publish(MQTT_TOPIC_DATA, json_string.c_str());
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Topic: ");
    Serial.println(topic);

    Serial.print("Payload: ");
    Serial.println((char *)payload);

    // Select appropriate handler for received MQTT topic
    if (strcmp(topic, MQTT_TOPIC_DATA_REQUEST) == 0) {
        mqtt_handle_data_request(payload, length);
    }
}

void mqtt_connect() {
    Serial.println("Connecting to the MQTT broker ...");

    if (mqtt_client.connect(MQTT_ID)) {
        Serial.println("Connected");
    } else {
        Serial.println("Connection failed");
        delay(5000);
        return;
    };

    //mqtt_client.publish("test", "I'm alive!");

    // Subscribe to MQTT topics
    mqtt_client.subscribe(MQTT_TOPIC_DATA_REQUEST);
}

void setup() {
    pinMode(COMPRESSOR_RELAY_PIN, OUTPUT);
    digitalWrite(COMPRESSOR_RELAY_PIN, LOW);

    Serial.begin(9600);
    Serial.println("Start");

    Ethernet.begin(mac);
    mqtt_client = PubSubClient(MQTT_BROKER_HOST, MQTT_BROKER_PORT, mqtt_callback, ethernet_client);
}

void loop() {
    if (!mqtt_client.connected()) {
        mqtt_connect();
    }

    mqtt_client.loop();
}
