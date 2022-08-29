#include <Arduino.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

String dId = "";
String webhook_pass = "";
String webhook_endpoint = "http://192.168.0.109:3001/api/getdevicescredentials";
const char *mqtt_server = "192.168.0.109";

// PINS
#define led 2

// WiFi credentials
const char *wifi_ssid = "Tenda_982610";
const char *wifi_password = "mfQ7SSJh";

// Function definitions
void clear();

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  clear();

  WiFi.begin(wifi_ssid, wifi_password);
  Serial.println(underlinePurple + "\n\n\nWiFi CONNECTION STARTED" + fontReset + Purple);

  int counter = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter > 10)
    {
      Serial.print(" ⤵" + fontReset);
      Serial.print(Red + "\n\n    WiFi CONNECTION FAILED :( ");
      Serial.println(" -> RESTARTING..." + fontReset);
      delay(2000);
      ESP.restart();
    }
  }

  Serial.print("  ⤵" + fontReset);

  // Printing local ip
  Serial.println(boldGreen + "\n\n         WiFi Connection -> SUCCESS :)" + fontReset);
  Serial.print("\n         Local IP -> ");
  Serial.print(boldBlue);
  Serial.print(WiFi.localIP());
  Serial.println(fontReset);
}

void loop()
{
}

void clear()
{
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");
}