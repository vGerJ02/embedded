#include <WiFi.h>
#include <PubSubClient.h>

// LCD
#define maxX 127
#define maxY 63
#define LETTER_WIDTH 6   // widht in pixels of letter + 1 of separation
#define LETTER_HEIGHT 7  // 6 + 1 of separation
const char *lcd_temperature_label = "TEMPERATURE  ";
const char *lcd_humidity_label = "HUMIDITY  ";
bool showing_temp = false;

// WIFI / MQTT
const char *ssid = "Al nord fa fred.";
const char *password = "hoolaa73";
const char *mqtt_server = "5.196.78.28";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.print("Connection to ");
  Serial.print(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.print("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_client() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  showWaiting();
}

void reconnect() {
  while (!client.connected()) {
    clear_screen();
    Serial.print("Attempting MQTT connection... ");
    if (client.connect("ESP32Client-MC")) {
      Serial.print("connected");

      client.subscribe("HTC/data");
      client.subscribe("USC/data");
      showWaiting();
    } else {
      escribir_LCD("failed, rc=");
      Serial.print(client.state());
      escribir_LCD(" retry in 5 seconds");
      delay(5000);
    }
  }
}

void clear_screen() {
  Serial.print((char)0x7C);
  Serial.print((char)0x00);
  delay(20);
}

void callback(char *topic, byte *payload, unsigned int length) {
  String data = "";
  for (int i = 0; i < length; i++) {
    data += (char)payload[i];
    // Serial.print((char)payload[i]);
  }

  if (String(topic).equals("USC/data")) {
    // Convert str to float
    checkDistance(data.toFloat());
    // clear_screen();
    // Serial.print("Distance: " + data);
  } else if (String(topic).equals("HTC/data")) {
    // Split the string to extract the two floats
    int delimiterIndex = data.indexOf(';');
    if (delimiterIndex != -1) {
      String tempString = data.substring(0, delimiterIndex);
      String humString = data.substring(delimiterIndex + 1);

      float temp = tempString.toFloat();
      float hum = humString.toFloat();

      if (!showing_temp) {
        lcd_temphum(temp, hum);
      } else {
        lcd_update_temphum(temp, hum);
      }

    } else {
      Serial.println("Error: Invalid payload format");
    }
  }
}

void checkDistance(float distance) {
  if (distance > 0.00 && distance < 10.00) {
    client.publish("MC/requestHTC", "1");
  } else {
    client.publish("MC/requestHTC", "0");
    if (showing_temp)
      showWaiting();
  }
}

void move_cursor(int x, int y) {  //x de 0 a 127
  Serial.print((char)0x7C);       //y de 0 a  63
  Serial.print((char)0x18);
  Serial.print((char)x);
  Serial.print((char)0x7C);
  Serial.print((char)0x19);
  Serial.print((char)y);
  delay(20);
}

void lcd_temphum(float temp, float hum) {
  clear_screen();
  move_cursor(0, 63);
  Serial.print(lcd_temperature_label);
  Serial.print(temp);
  Serial.print(" C ");
  Serial.print(lcd_humidity_label);
  Serial.print(hum);
  Serial.print("%     ");
  showing_temp = true;
}

void lcd_update_temphum(float temp, float hum) {
  // Move cursor to value and replace
  int x = strlen(lcd_temperature_label);
  move_cursor(x * LETTER_WIDTH, 63);
  Serial.print(temp);
  x = strlen(lcd_humidity_label);
  move_cursor(x * LETTER_WIDTH, 62 - LETTER_HEIGHT);
  Serial.print(hum);
}


void setup_lcd() {
  delay(1200);  ///wait for the one second spalsh screen before anything is sent to the LCD.
  clear_screen();
  delay(10);
}

void escribir_LCD(char *data) {
  Serial.print(data);
  delay(20);
}

void showWaiting() {
  showing_temp = false;
  clear_screen();

  escribir_LCD("       Welcome       ");
  escribir_LCD("     Min distance    ");
  escribir_LCD("        10.00        ");

  move_cursor(8 * 5 + 1, 63);
  escribir_LCD("BACK");
}


void setup() {
  Serial.begin(115200);
  setup_lcd();
  setup_wifi();  // Connect wifi
  setup_client();
}

void loop() {
  delay(2000);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
