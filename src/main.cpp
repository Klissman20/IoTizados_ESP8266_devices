#include <Arduino.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

String dId = "2343452";
String webhook_pass = "KBl2HWGRza";
String webhook_endpoint = "/api/getdevicecredentials";
String host = "192.168.0.109";
int api_port = 3001;
const char *mqtt_server = "192.168.0.109";

WiFiClient client;
DynamicJsonDocument mqtt_data_doc(1024);

// PINS
#define led 2

// WiFi credentials
const char *wifi_ssid = "Tenda_982610";
const char *wifi_password = "mfQ7SSJh";

// Function definitions
void clear();
bool get_mqtt_credentials();

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

  get_mqtt_credentials();
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

bool get_mqtt_credentials()
{

  Serial.print(underlinePurple + "\n\n\nGetting MQTT Credentials from WebHook" + fontReset + Purple + "  ⤵");
  delay(1000);

  String toSend = "dId=" + dId + "&password=" + webhook_pass;

  HTTPClient http;
  http.begin(client, host, api_port, webhook_endpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int response_code = http.POST(toSend);

  if (response_code < 0)
  {
    Serial.print(boldRed + "\n\n         Error Sending Post Request :( " + fontReset);
    http.end();
    return false;
  }

  if (response_code != 200)
  {
    Serial.print(boldRed + "\n\n         Error in response :(   e-> " + fontReset + " " + response_code);
    http.end();
    return false;
  }

  if (response_code == 200)
  {
    String responseBody = http.getString();

    Serial.print(boldGreen + "\n\n         Mqtt Credentials Obtained Successfully :) " + fontReset);
    Serial.print("\n\n" + responseBody);
    deserializeJson(mqtt_data_doc, responseBody);
    http.end();
    delay(2000);

    /*String mqtt_topic = mqtt_data_doc["topic"];
    int freq = mqtt_data_doc["variables"][1]["variableSendFreq"];
    Serial.print("\n\n" + mqtt_topic);
    Serial.print("\n\n" + String(freq));*/

    return true;
  }

  return false;
}