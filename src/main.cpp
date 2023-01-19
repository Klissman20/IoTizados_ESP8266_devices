#include <Arduino.h>
#include "Colors.h"
#include "IoTicosSplitter.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

String dId = "2943745658";
String webhook_pass = "Nn6SvqEERd";
String webhook_endpoint = "/api/getdevicecredentials";
String host = "192.168.0.106";
int api_port = 3001;
const char *mqtt_server = "192.168.0.106";

// GLOBAL Vars
WiFiClient espclient;
PubSubClient client(espclient);
DynamicJsonDocument mqtt_data_doc(1024);
IoTicosSplitter splitter;
long varsLastSend[20];
long lastReconnectAttemp = 0;
long lastStats = 0;
int prev_temp = 0;
// int prev_hum = 0;
String last_received_msg = "";
String last_received_topic = "";

// PINS
#define led 2
#define light 16

// WiFi credentials
const char *wifi_ssid = "Tenda_982610";
const char *wifi_password = "mfQ7SSJh";

// Function definitions
void clear();
bool get_mqtt_credentials();
void check_mqtt_connection();
bool reconnect();
void process_sensors();
void process_actuators();
void send_data_to_broker();
void callback(char *topic, byte *payload, unsigned int length);
void process_incoming_msg(String topic, String incoming);
void print_stats();

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(light, OUTPUT);
  digitalWrite(led, LOW); // LOW: on led   HIGH: off led
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

  client.setCallback(callback);
}

void loop()
{
  check_mqtt_connection();
}

void process_sensors()
{
  // get temp simulation
  int temp = random(1, 100);
  mqtt_data_doc["variables"][0]["last"]["value"] = temp;

  mqtt_data_doc["variables"][3]["last"]["value"] = (LOW == digitalRead(led));

  // save temp?
  int dif = temp - prev_temp;
  if (dif < 0)
  {
    dif *= -1;
  }

  if (dif >= 40)
  {
    mqtt_data_doc["variables"][0]["last"]["save"] = 1;
  }
  else
  {
    mqtt_data_doc["variables"][0]["last"]["save"] = 0;
  }

  prev_temp = temp;

  /*
  // get humidity simulation
  int hum = random(1, 50);
  mqtt_data_doc["variables"][1]["last"]["value"] = hum;

  // save hum?
  dif = hum - prev_hum;
  if (dif < 0)
  {
    dif *= -1;
  }

  if (dif >= 20)
  {
    mqtt_data_doc["variables"][1]["last"]["save"] = 1;
  }
  else
  {
    mqtt_data_doc["variables"][1]["last"]["save"] = 0;
  }

  prev_hum = hum;*/

  mqtt_data_doc.garbageCollect();
}

void process_actuators()
{

  if (mqtt_data_doc["variables"][1]["last"]["value"] == "true")
  {
    digitalWrite(led, LOW);
    digitalWrite(light, LOW);
    mqtt_data_doc["variables"][1]["last"]["value"] = "";
    varsLastSend[3] = 0;
  }
  else if (mqtt_data_doc["variables"][2]["last"]["value"] == "false")
  {
    digitalWrite(led, HIGH);
    digitalWrite(light, HIGH);
    mqtt_data_doc["variables"][2]["last"]["value"] = "";
    varsLastSend[3] = 0;
  }
}

//***********************
//*****  TEMPLATE⤵  ****
//***********************

void process_incoming_msg(String topic, String incoming)
{
  last_received_topic = topic;
  last_received_msg = incoming;
  String variable = splitter.split(topic, '/', 2);

  for (unsigned int i = 0; i < mqtt_data_doc["variables"].size(); i++)
  {

    if (mqtt_data_doc["variables"][i]["variable"] == variable)
    {

      DynamicJsonDocument doc(512);
      deserializeJson(doc, incoming);
      mqtt_data_doc["variables"][i]["last"] = doc;

      long counter = mqtt_data_doc["variables"][i]["counter"];
      counter++;
      mqtt_data_doc["variables"][i]["counter"] = counter;
    }
  }
  process_actuators();
  mqtt_data_doc.garbageCollect();
  // serializeJsonPretty(mqtt_data_doc, Serial);
}

void callback(char *topic, byte *payload, unsigned int length)
{

  String incoming = "";
  for (unsigned int i = 0; i < length; i++)
  {
    incoming += (char)payload[i];
  }

  incoming.trim();

  process_incoming_msg(String(topic), incoming);
  // Serial.println(incoming);
  // Serial.println(String(topic));
}

void send_data_to_broker()
{

  long now = millis();

  for (unsigned int i = 0; i < mqtt_data_doc["variables"].size(); i++)
  {

    if (mqtt_data_doc["variables"][i]["variableType"] == "output")
    {
      continue;
    }

    int freq = mqtt_data_doc["variables"][i]["variableSendFreq"];

    if (now - varsLastSend[i] > freq * 1000)
    {
      varsLastSend[i] = millis();

      String str_root_topic = mqtt_data_doc["topic"];
      String str_variable = mqtt_data_doc["variables"][i]["variable"];
      String topic = str_root_topic + str_variable + "/sdata";

      String toSend = "";

      serializeJson(mqtt_data_doc["variables"][i]["last"], toSend);

      client.publish(topic.c_str(), toSend.c_str());

      // STATS
      long counter = mqtt_data_doc["variables"][i]["counter"];
      counter++;
      mqtt_data_doc["variables"][i]["counter"] = counter;
    }
  }
}

void check_mqtt_connection()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(Red + "\n\n         Ups WiFi Connection Failed :( ");
    Serial.println(" -> Restarting..." + fontReset);
    delay(15000);
    ESP.restart();
  }

  if (!client.connected())
  {

    long now = millis();

    if (now - lastReconnectAttemp > 5000)
    {
      lastReconnectAttemp = millis();
      if (reconnect())
      {
        lastReconnectAttemp = 0;
      }
    }
  }
  else
  {
    client.loop();
    process_sensors();
    // process_actuators();
    send_data_to_broker();
    print_stats();
  }
}

bool reconnect()
{

  if (!get_mqtt_credentials())
  {
    Serial.println(boldRed + "\n\n      Error getting mqtt credentials :( \n\n RESTARTING IN 10 SECONDS");
    Serial.println(fontReset);
    delay(10000);
    ESP.restart();
  }

  // Setting up Mqtt Server
  client.setServer(mqtt_server, 1883);

  Serial.print(underlinePurple + "\n\n\nTrying MQTT Connection" + fontReset + Purple + "  ⤵");

  String str_client_id = "device_" + dId + "_" + random(1, 9999);
  const char *username = mqtt_data_doc["username"];
  const char *password = mqtt_data_doc["password"];
  String str_topic = mqtt_data_doc["topic"];

  if (client.connect(str_client_id.c_str(), username, password))
  {
    Serial.print(boldGreen + "\n\n         Mqtt Client Connected :) " + fontReset);
    delay(2000);

    client.subscribe((str_topic + "+/actdata").c_str());
    return true;
  }
  else
  {
    Serial.print(boldRed + "\n\n         Mqtt Client Connection Failed :( " + fontReset);
    return false;
  }
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
  http.begin(espclient, host, api_port, webhook_endpoint);
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
    // Serial.print("\n\n" + responseBody);
    deserializeJson(mqtt_data_doc, responseBody);
    http.end();
    delay(2000);
    /*String mqtt_user = mqtt_data_doc["username"];
    String mqtt_pass = mqtt_data_doc["password"];
    String mqtt_topic = mqtt_data_doc["topic"];
    int freq = mqtt_data_doc["variables"][1]["variableSendFreq"];
    Serial.print("\n\n" + mqtt_user);
    Serial.print("\n\n" + mqtt_pass);
    Serial.print("\n\n" + mqtt_topic);
    Serial.print("\n\n" + String(freq));*/

    return true;
  }

  return true;
}

void print_stats()
{
  long now = millis();

  if (now - lastStats > 2000)
  {
    lastStats = millis();
    clear();

    Serial.print("\n");
    Serial.print(Purple + "\n╔══════════════════════════╗" + fontReset);
    Serial.print(Purple + "\n║       SYSTEM STATS       ║" + fontReset);
    Serial.print(Purple + "\n╚══════════════════════════╝" + fontReset);
    Serial.print("\n\n");
    Serial.print("\n\n");

    Serial.print(boldCyan + "#" + " \t Name" + " \t\t Var" + " \t\t Type" + " \t\t Count" + " \t\t Last V" + fontReset + "\n\n");

    for (unsigned int i = 0; i < mqtt_data_doc["variables"].size(); i++)
    {

      String variableFullName = mqtt_data_doc["variables"][i]["variableFullName"];
      String variable = mqtt_data_doc["variables"][i]["variable"];
      String variableType = mqtt_data_doc["variables"][i]["variableType"];
      String lastMsg = mqtt_data_doc["variables"][i]["last"];
      long counter = mqtt_data_doc["variables"][i]["counter"];

      Serial.println(String(i) + " \t " + variableFullName.substring(0, 5) + " \t\t " + variable.substring(0, 10) + " \t " + variableType.substring(0, 5) + " \t\t " + String(counter).substring(0, 10) + " \t\t " + lastMsg);
    }

    Serial.print(boldGreen + "\n\n Free RAM -> " + fontReset + ESP.getFreeHeap() + " Bytes");

    Serial.print(boldGreen + "\n\n Last Incomming Msg -> " + fontReset + last_received_msg);
  }
}