#include <UIPEthernet.h>
#include <PubSubClient.h>

#define MQTT_TOPIC_DATA_REQUEST "sensor/update_request"
#define MQTT_TOPIC_DATA "sensor/update"
#define MQTT_TOPIC_DATA_UPDATE_INTERVAL "sensor/update_interval"

#define MQTT_BROKER_HOST "192.168.2.116"
#define MQTT_BROKER_PORT 1883

#define MQTT_ID "arduino"

#define COMPRESSOR_RELAY_PIN 2
#define PRESSURE_SENSOR_PIN 0
#define MOISTURE_SENSOR_PIN 1

#define DEFAULT_DATA_UPDATE_INTERVAL 60000
#define MIN_DATA_UPDATE_INTERVAL 30000

byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x31 };
IPAddress ip(192,168,2,190);

EthernetClient ethernet_client;
PubSubClient mqtt_client;

unsigned long update_interval = DEFAULT_DATA_UPDATE_INTERVAL;

// Convert ascii data to unsigned long (32b)
unsigned long data_to_long(unsigned char *data, unsigned length, bool *err) {
    *err = false;
    unsigned long x = 0;
    for (unsigned i = 0; i < length; i++) {
        char digit = data[i] - '0';
        if (digit > 9 || digit < 0) {
            *err = true;
            return 0;
        }
        x = x * 10 + digit;
    }
    return x;
}

// Get water level in meters
float get_water_level() {
    // Turn on the compressor for 5 seconds
    digitalWrite(COMPRESSOR_RELAY_PIN, HIGH);
    delay(5000);
    digitalWrite(COMPRESSOR_RELAY_PIN, LOW);

    // Let the pressure stabilize
    delay(2000);

    float pressure_sensor_value = analogRead(PRESSURE_SENSOR_PIN);
    float pressure = ((pressure_sensor_value/1024.0)-0.0312)/0.000009;
    // water level = kPa / 10
    return pressure / 10000;
}

// Get soil moisture level in %
float get_moisture_level() {
    return (analogRead(MOISTURE_SENSOR_PIN) / 1024.0f) * 100;
}

void mqtt_update_data() {
    float water_level = get_water_level();
    float moisture_level = get_moisture_level();

    // Build the JSON string
    // {
    //   'water_level': xyz
    //   'moisture_level': xyz
    // }
    String json_string = String("{'water_level':" + String(water_level, 1) + ",'moisture_level':" + String(moisture_level, 1) + "}");

    //Serial.println(json_string);

    mqtt_client.publish(MQTT_TOPIC_DATA, json_string.c_str());
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    //Serial.print("Topic: "); Serial.println(topic);
    //Serial.print("Payload: "); Serial.println((char *)payload);

    // Select appropriate handler for received MQTT topic
    // Data update request
    if (strcmp(topic, MQTT_TOPIC_DATA_REQUEST) == 0) {
        mqtt_update_data();
    // Data update interval change
    } else if (strcmp(topic, MQTT_TOPIC_DATA_UPDATE_INTERVAL) == 0) {
        bool err;
        unsigned long new_update_interval = data_to_long(payload, length, &err);
        if (!err && new_update_interval >= MIN_DATA_UPDATE_INTERVAL) {
            //Serial.print("Updating data update interval to: "); Serial.println(new_update_interval);
            update_interval = new_update_interval;
        }
    }
}

void mqtt_connect() {
    for (;;) {
        Serial.println(F("C:"));
        if (mqtt_client.connect(MQTT_ID)) {
            Serial.println(F("C+"));
            break;
        } else {
            Serial.println("C-");
            delay(5000);
        };
    }

    mqtt_client.subscribe(MQTT_TOPIC_DATA_REQUEST);
    mqtt_client.subscribe(MQTT_TOPIC_DATA_UPDATE_INTERVAL);
}


void setup() {
    pinMode(COMPRESSOR_RELAY_PIN, OUTPUT);
    digitalWrite(COMPRESSOR_RELAY_PIN, LOW);

    Serial.begin(9600);
    Serial.println("S");

    Ethernet.begin(mac, ip);
    mqtt_client = PubSubClient(MQTT_BROKER_HOST, MQTT_BROKER_PORT, mqtt_callback, ethernet_client);
}

void loop() {
    if (!mqtt_client.connected()) {
        mqtt_connect();
    }

    unsigned long current_time = millis();
    static unsigned long last_update_time = 0;
    // Handle millis() rollover after 50 days of running
    if (last_update_time > current_time) {
        // TODO something better
        last_update_time = 0;
    }

    if (current_time - last_update_time >= update_interval) {
        last_update_time = current_time;

        mqtt_update_data();
    }

    mqtt_client.loop();
}
