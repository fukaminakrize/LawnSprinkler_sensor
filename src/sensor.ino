#include <UIPEthernet.h>
#include <UIPClient.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_DATA_REQUEST "dataRequest"

#define MQTT_BROKER_HOST "192.168.0.123"
#define MQTT_BROKER_PORT 1883

#define MQTT_ID "arduino"

byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x31 };
IPAddress ip("192.168.0.179");

EthernetClient ethernet_client;
PubSubClient mqtt_client;

void mqtt_handle_data_request(byte *payload, unsigned int length) {
    // Zacat zbierat data
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Topic: ");
    Serial.println(topic);

    Serial.print("Payload: ");
    Serial.println((char *)payload);

    // Select appropriate handler for received MQTT topic
    if (strcmp(topic, TOPIC_DATA_REQUEST) == 0) {
        mqtt_handle_data_request(payload, length);
    }
}

void mqtt_connect() {
    Serial.println("Connecting to MQTT broker ...");

    mqtt_client.connect(MQTT_ID);

    //mqtt_client.publish("test", "I'm alive!");

    // Subscribe to MQTT topics
    mqtt_client.subscribe(MQTT_TOPIC_DATA_REQUEST);
}

void setup() {
    Serial.begin(9600);
    Serial.println("Start");

    Ethernet.begin(mac, ip);
    mqtt_client = PubSubClient(MQTT_BROKER_HOST, MQTT_BROKER_PORT, mqtt_callback, ethernet_client);
}

void loop() {
    if (!mqtt_client.connected()) {
        mqtt_connect();
    }

    mqtt_client.loop();
}
