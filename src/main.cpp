#include <Arduino.h>

#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <stdarg.h>

#include "pages.h"

#include "serial.h"

#define RESET_BUTTON_PIN 0
#define STATUS_LED_PIN 2

void handleRoot();
void handleLogin();
void handleNotFound();

String readFromSerialIfAvailable();
void processSerialCommand(const String &cmd);
void configState(const String &message);
void operationState();
void idleState();
void checkConnectionState();
bool sendDataToServer(const String &message);
bool connectToNetwork(const String &ssid, const String &password);

void handleButton();
void displayStatus();

ESP8266WebServer server(80);

String g_ssid = "";
String g_password = "";

bool resetBtnPrestate = false;
bool wifiStatus = false;

void processSerialCommand(const String &cmd)
{
  if (cmd.length() == 0)
  {
    return;
  }

  // #678;58;25 OR
  if (cmd.indexOf("#") == 0)
  {
    // ...
  }
}

void configState()
{
  WiFi.disconnect();
  debugSerialPrintf("Setting soft-AP ... ");
  boolean result = WiFi.softAP("CyberPlant echo - station", "");
  if (!result)
  {
    debugSerialPrintf("Failed!");
    return;
  }

  debugSerialPrintf("Ready");
  IPAddress IP = WiFi.softAPIP();
  debugSerialPrintf("AP IP address: ");
  debugSerialPrintf(IP.toString().c_str());

  server.begin();

  debugSerialPrintf("HTTP server started");

  wifiStatus = true;
}

void operationState()
{
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);

  bool isConnected = connectToNetwork(g_ssid, g_password);

  if (isConnected)
  {
    debugSerialPrintf("Connection established!");
    debugSerialPrintf("IP address:\t");
    debugSerialPrintf(WiFi.localIP().toString().c_str());

    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    debugSerialPrintf("#WIFI_CONNECTED!%s", WiFi.localIP().toString().c_str());
  }
  else
  {
    debugSerialPrintf("#WIFI_CONNECTION_FAILED");
  }
}

bool connectToNetwork(const String &ssid, const String &password)
{
  if (ssid == "" && password == "")
  {
    WiFi.begin();
  }
  else
  {
    WiFi.begin(ssid, password);
  }

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    debugSerialPrintf("%d", ++i);
    debugSerialPrintf(" ");

    if (i == 10)
    {
      return false;
    }
  }

  return true;
}

void idleState()
{
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
}

// 2#http://192.168.88.252:3000/insert!{"sensor_id":"value1", "command":0, "temperature":1.1, "humidity":2.22, "soil_moisture":3.33}
bool sendDataToServer(const String &message)
{
  WiFiClient client;
  HTTPClient http;

  String serverName = message.substring(0, message.indexOf('!'));
  String postMessage = message.substring(message.indexOf('!') + 1, message.length());

  http.begin(client, serverName);

  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(postMessage);

  debugSerialPrintf("HTTP Response code: ");
  debugSerialPrintf("%d", httpResponseCode);

  http.end();

  if (httpResponseCode >= 200 && httpResponseCode <= 299)
  {
    debugSerialPrintf("#TRANSPORT_OK");

    return true;
  }
  else
  {
    debugSerialPrintf("#TRANSPORT_FAILED!%d", httpResponseCode);

    return false;
  }

  return false;
}

void handleRoot()
{
  String s = MAIN_page;
  server.send(200, "text/html", s);
}

void handleDone()
{
  if (!server.hasArg("ssid") || !server.hasArg("password") || server.arg("ssid") == NULL || server.arg("password") == NULL)
  {
    String s = ERROR_page;
    server.send(400, "text/html", s);

    debugSerialPrintf("#CONFIG_FAILED");
    return;
  }

  if (server.hasArg("sm-air") || server.hasArg("sm-water") || server.arg("sm-air") != NULL || server.arg("sm-water") != NULL)
  {
    debugSerialPrintf("#CUSTOM_CFG!%s!%s", server.arg("sm-air").c_str(), server.arg("sm-water").c_str());
  }

  g_ssid = server.arg("ssid");
  g_password = server.arg("password");

  String s = DONE_page;
  server.send(200, "text/html", s);

  debugSerialPrintf("#CONFIG_DONE");

  delay(3000);
  operationState();
}

void handleNotFound()
{
  String s = NOT_FOUND_page;
  server.send(200, "text/html", s);
}

String readFromSerialIfAvailable()
{
  String responseBuffer;
  char charIn;

  unsigned long startTime = millis();

  while ((millis() - startTime) < 3000)
  {
    if (Serial.available())
    {
      charIn = Serial.read();
      responseBuffer += charIn;
    }
    if (responseBuffer.endsWith("\r\n\0"))
    {
      responseBuffer.trim();
      return responseBuffer;
    }
  }
  if (!responseBuffer.endsWith("\r\n\0"))
  {
    responseBuffer.trim();
    return "";
  }

  return "";
}

void handleButton()
{
  int resetValue = digitalRead(RESET_BUTTON_PIN);

  if (resetValue == HIGH)
  {
    if (!resetBtnPrestate)
    {
      configState();
    }
  }
  else
  {
    resetBtnPrestate = false;
  }
}

void displayStatus()
{
  if (wifiStatus)
  {
    digitalWrite(STATUS_LED_PIN, LOW);
  }
  else
  {
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
}

void setup()
{
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_POST, handleDone);
  server.onNotFound(handleNotFound);

  pinMode(RESET_BUTTON_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  debugSerialPrintf("#MODULE_READY");
}

void loop()
{
  server.handleClient();

  String serialCmd = readFromSerialIfAvailable();
  processSerialCommand(serialCmd);

  handleButton();
  displayStatus();
}
