#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // https://randomnerdtutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/
#include <stdarg.h>

#include "pages.h"

#include "serial.h"

#define RESET_BUTTON_PIN 0
#define STATUS_LED_PIN 2

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

AsyncWebServer server(80);

String g_ssid = "";
String g_password = "";

bool resetBtnPrestate = false;
bool wifiStatus = false;

String soilMoistureValue = "666";

String getMessageElement(const String &data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void processSerialCommand(const String &cmd)
{
  if (cmd.length() == 0)
  {
    return;
  }

  // #678;58;25
  if (cmd.indexOf("#") == 0)
  {
    soilMoistureValue = getMessageElement(cmd, ';', 0).substring(1, cmd.indexOf(";"));
  }
}

void configState()
{
  WiFi.disconnect();
  serialPrintf("Setting soft-AP ... ");
  boolean result = WiFi.softAP("CyberPlant echo - station", "");
  if (!result)
  {
    serialPrintf("Failed!");
    return;
  }

  serialPrintf("Ready");
  IPAddress IP = WiFi.softAPIP();
  serialPrintf("AP IP address: ");
  serialPrintf(IP.toString().c_str());

  serialPrintf("HTTP server started");

  wifiStatus = true;
}

void operationState()
{
//  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);

  bool isConnected = connectToNetwork(g_ssid, g_password);

  if (isConnected)
  {
    serialPrintf("Connection established!");
    serialPrintf("IP address:\t");
    serialPrintf(WiFi.localIP().toString().c_str());

    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    serialPrintf("#WIFI_CONNECTED!%s", WiFi.localIP().toString().c_str());
  }
  else
  {
    serialPrintf("#WIFI_CONNECTION_FAILED");
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
    serialPrintf("%d", ++i);
    serialPrintf(" ");

    if (i == 10)
    {
      return false;
    }
  }

  return true;
}

void idleState()
{
//  server.stop();
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

  serialPrintf("HTTP Response code: ");
  serialPrintf("%d", httpResponseCode);

  http.end();

  if (httpResponseCode >= 200 && httpResponseCode <= 299)
  {
    serialPrintf("#TRANSPORT_OK");

    return true;
  }
  else
  {
    serialPrintf("#TRANSPORT_FAILED!%d", httpResponseCode);

    return false;
  }

  return false;
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

  if (resetValue == LOW)
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
    digitalWrite(STATUS_LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String s = MAIN_page;
      request->send(200, "text/html", s);
  });

  
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("ssid") || !request->hasParam("password"))
    {
      String s = ERROR_page;
      request->send(400, "text/html", s);
  
      serialPrintf("#CONFIG_FAILED");
      return;
    }
  
    if (request->hasParam("sm-air") && request->hasParam("sm-water"))
    {
      String airValue = request->getParam("sm-air")->value();
      String waterValue = request->getParam("sm-water")->value();

      char buff[15];
      sprintf(buff, "#CUSTOM_CFG!%s!%s", airValue.c_str(), waterValue.c_str());
      Serial.println(buff);
    }
  
    g_ssid = request->getParam("ssid")->value();
    g_password = request->getParam("password")->value();
  
  
    String s = DONE_page;
    request->send(200, "text/html", s);
  
    serialPrintf("#CONFIG_DONE");
  
    delay(3000);
    operationState();
  });

  server.on("/soil-moisture-value", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", soilMoistureValue);
  });
  
  server.onNotFound(notFound);

  server.begin();

  pinMode(RESET_BUTTON_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  serialPrintf("#MODULE_READY");
}

void loop()
{
  String serialCmd = readFromSerialIfAvailable();
  processSerialCommand(serialCmd);

  handleButton();
  displayStatus();
}
