#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED_PIN D4  //LED PIN

// Wifi and mqtt
const char* ssid = "Al nord fa fred.";
const char* password = "hoolaa73";
const char* mqtt_server = "5.196.78.28";

const float min_distance = 10.00;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034

const int trigPin = 12;
const int echoPin = 14;

long duration;
float distanceCm;

void setup_wifi() {
  delay(10);
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
    if (client.connect("ESP8266Client-USC")) {
      Serial.println("connected");
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
}

void send_data() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY / 2;

  String distanceStr = String(distanceCm, 2);

  Serial.print("Distance (cm): " + distanceStr);
  Serial.println();

  if (distanceCm < min_distance) {
    digitalWrite(LED_PIN, HIGH);  // Turn LED on
  } else {
    digitalWrite(LED_PIN, LOW);  // Turn LED off
  }

  client.publish("USC/data", distanceStr.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Ensure the LED is off at startup
  setup_wifi();
  setup_client();

  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input
}

void loop() {

  if (client.connected()) {
    send_data();
  } else {
    reconnect();
  }

  client.loop();  // keep the connection alive

  delay(3000);
}
