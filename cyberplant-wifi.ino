#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // https://randomnerdtutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/
#include "AsyncJson.h"
#include "ArduinoJson.h" // https://github.com/bblanchon/ArduinoJson
#include <stdarg.h>

#include "pages.h"

#define RESET_BUTTON_PIN 0
#define STATUS_LED_PIN 2

String readFromSerialIfAvailable();
void processSerialCommand(const String &cmd);
void configState(const String &message);
void operationState();
void checkConnectionState();
bool sendDataToServer(const String &message);
bool connectToNetwork(const String &ssid, const String &password);

void handleButton();
void displayStatus();
void wifiStatusLEDTurnON();
void wifiStatusLEDTurnOFF();

AsyncWebServer server(80);

String g_ssid;
String g_password;

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

void blinkLed(int d) {
  wifiStatusLEDTurnON();
  delay(d);
  wifiStatusLEDTurnOFF();
  delay(d);
}

void configState()
{
  WiFi.disconnect();
  Serial.println("Setting soft-AP ... ");
  boolean result = WiFi.softAP("CyberPlant echo - station", "");
  if (!result)
  {
    Serial.println("Failed!");
    wifiStatusLEDTurnON();
    return;
  }

  Serial.println("Ready");
  IPAddress IP = WiFi.softAPIP();
  Serial.println("AP IP address: ");
  Serial.println(IP.toString().c_str());

  Serial.println("HTTP server started");

  blinkLed(200);
  blinkLed(200);
  blinkLed(200);
}

void operationState()
{
  //  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);

  bool isConnected = connectToNetwork(g_ssid, g_password);

  if (isConnected)
  {
    Serial.println("Connection established!");
    Serial.println("IP address:\t");
    Serial.println(WiFi.localIP().toString().c_str());

    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    Serial.print("#WIFI_CONNECTED!");
    Serial.println(WiFi.localIP().toString());

    wifiStatusLEDTurnOFF();
  }
  else
  {
    Serial.println("#WIFI_CONNECTION_FAILED");
    wifiStatusLEDTurnON();
  }


  displayStatus();
}

bool connectToNetwork(const String& ssid, const String& password)
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
    Serial.print(++i);
    Serial.print(" ");

    if (i == 10)
    {
      return false;
    }
  }

  return true;
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

  Serial.println("HTTP Response code: ");
  Serial.println(httpResponseCode);

  http.end();

  if (httpResponseCode >= 200 && httpResponseCode <= 299)
  {
    Serial.println("#TRANSPORT_OK");

    return true;
  }
  else
  {
    Serial.print("#TRANSPORT_FAILED!");
    Serial.println(httpResponseCode);

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

void wifiStatusLEDTurnON() {
  wifiStatus = false;
  displayStatus();
}

void wifiStatusLEDTurnOFF() {
  wifiStatus = true;
  displayStatus();
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

bool tryToConnectToWiFi = false;

void setup()
{
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    String s = MAIN_page;
    request->send(200, "text/html", s);
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest * request, JsonVariant & json) {
    StaticJsonDocument<200> jsonObj = json.as<JsonObject>();

    if (!jsonObj.containsKey("ssid") || !jsonObj.containsKey("password"))
    {
      AsyncWebServerResponse *response = request->beginResponse(400);
      request->send(response);
      return;
    }

    String ssid = jsonObj["ssid"];
    String pass = jsonObj["password"];

    if (ssid.length() == 0 || pass.length() == 0) {
      AsyncWebServerResponse *response = request->beginResponse(400);
      request->send(response);
      return;
    }

    g_ssid = ssid;
    g_password = pass;

    if (jsonObj.containsKey("sm-air") && jsonObj.containsKey("sm-water"))
    {
      String airValue = jsonObj["sm-air"];
      String waterValue = jsonObj["sm-water"];

      char buff[15];
      sprintf(buff, "#CUSTOM_CFG!%s!%s", airValue.c_str(), waterValue.c_str());
      Serial.println(buff);
    }

    AsyncWebServerResponse *response = request->beginResponse(200);
    request->send(response);
  });
  server.addHandler(handler);

  server.on("/done", HTTP_GET, [](AsyncWebServerRequest * request) {
    tryToConnectToWiFi = true;

    String s = DONE_page;
    request->send(200, "text/html", s);
  });

  server.on("/error", HTTP_GET, [](AsyncWebServerRequest * request) {
    String s = ERROR_page;
    request->send(400, "text/html", s);
  });

  server.on("/soil-moisture-value", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", soilMoistureValue);
  });

  server.onNotFound(notFound);

  server.begin();

  pinMode(RESET_BUTTON_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  Serial.println("#MODULE_READY");

  tryToConnectToWiFi = true;
}

void loop()
{
  String serialCmd = readFromSerialIfAvailable();
  processSerialCommand(serialCmd);

  handleButton();
  displayStatus();

  if (tryToConnectToWiFi) {
    tryToConnectToWiFi = false;
    operationState();
  }
}
