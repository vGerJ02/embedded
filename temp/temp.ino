#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DPIN 4  // GPIO D2
// LED pin
#define LED_PIN D5
#define DTYPE DHT11

// Wifi and mqtt
const char* ssid = "Al nord fa fred.";
const char* password = "hoolaa73";
const char* mqtt_server = "5.196.78.28";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

DHT dht(DPIN, DTYPE);
bool isSendingData = false;

void setup_wifi() {
  delay(10);
  digitalWrite(LED_PIN, LOW);  // Turn LED off
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_client() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client-HTC")) {
      Serial.println("connected");

      client.subscribe("MC/requestHTC");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic).equals("MC/requestHTC")) {
    // Check if payload is '1' or '0'
    if (length == 1 && (payload[0] == '1' || payload[0] == '0')) {
      if (payload[0] == '1') {
        isSendingData = true;
      } else if (payload[0] == '0') {
        Serial.print("Stop sending data.");
        isSendingData = false;
      }
    } else {
      Serial.println("Invalid payload for MC/requestHTC");
    }
  }
}

String getTemperatureAndHumidity() {
  float tc = dht.readTemperature(false);  //Read temperature in C
  float hu = dht.readHumidity();

  if (isnan(tc) || isnan(hu)) {
    return "Failed to read from DHT sensor.";
  }

  return String(tc) + ";" + String(hu);
}

void sendTemperatureAndHumidity() {
  if (isSendingData) {
    Serial.print("Sending data...");
    digitalWrite(LED_PIN, HIGH);  // Turn LED on
    client.publish("HTC/data", getTemperatureAndHumidity().c_str());
  } else {
    digitalWrite(LED_PIN, LOW);  // Turn LED off
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Ensure the LED is off at startup
  dht.begin();                 // Start sensor
  setup_wifi();                // Connect wifi
  setup_client();
}

void loop() {
  delay(2000);

  if (!client.connected()) {
    reconnect();
  } else {
    sendTemperatureAndHumidity();
  }
  client.loop();

  // Serial.print(getTemperatureAndHumidity());
}
