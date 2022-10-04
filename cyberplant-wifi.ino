#define DEVICE_ID "AlmafaMokus1"

const char HOST[] PROGMEM = {"http://192.168.88.207:3000/device/insert"};

#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPAsyncWebServer.h> // https://randomnerdtutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/
#include "AsyncJson.h"
#include "ArduinoJson.h" // https://github.com/bblanchon/ArduinoJson
#include <stdarg.h>
#include <EEPROM.h> // esp8266 has no EEPROM this is an interface to writing into the flash

#include "pages.h"

#define RESET_BUTTON_PIN 0
#define STATUS_LED_PIN 2

String readFromSerialIfAvailable();
void processSerialCommand(const String &cmd);
void configState(const String &message);
void operationState();
void checkConnectionState();
int sendDataToServer(const String &message);
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

void writeToken(const String token) {
  EEPROM.begin(512);
  EEPROM.write(0, byte(token.length()));

  for (int i = 0; i < token.length(); i++) {
    EEPROM.write(i + 1, token[i]);
  }

  EEPROM.commit();
  EEPROM.end();
}

String readToken() {
  EEPROM.begin(512);

  int tokenSize = EEPROM.read(0);

  String strText;
  for (int i = 0; i < tokenSize; i++)
  {
    char cc;
    EEPROM.get(i + 1, cc);
    strText = strText + cc;
  }

  EEPROM.end();
  return strText;
}

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

    if (wifiStatus) {
      String hum = getMessageElement(cmd, ';', 1);
      String temp = getMessageElement(cmd, ';', 2);

      StaticJsonDocument<400> doc;
      doc["device_id"] = DEVICE_ID;

      StaticJsonDocument<400> sensors;
      JsonArray arr = sensors.to<JsonArray>();

      JsonObject soilMoistureSensorData = arr.createNestedObject();
      soilMoistureSensorData["id"] = "soil_moisture_percent";
      soilMoistureSensorData["type"] = "int";
      soilMoistureSensorData["value"] = soilMoistureValue;

      JsonObject humiditySensorData = arr.createNestedObject();
      humiditySensorData["id"] = "humidity_percent";
      humiditySensorData["type"] = "float";
      humiditySensorData["value"] = hum;

      JsonObject temperatureSensorData = arr.createNestedObject();
      temperatureSensorData["id"] = "temperature_percent";
      temperatureSensorData["type"] = "float";
      temperatureSensorData["value"] = temp;

      doc["sensors"] = sensors;

      String jsonString;
      serializeJson(doc, jsonString);

      int requestStatusCode = sendDataToServer(HOST, jsonString);

      if (!(requestStatusCode >= 200 && requestStatusCode <= 299))
      {
        blinkLed(200);
        blinkLed(200);
      }
    }
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
  Serial.println("#1!0!0!0");

  WiFi.disconnect();
  boolean result = WiFi.softAP("CyberPlant echo - station", "");
  if (!result)
  {
    wifiStatusLEDTurnON();
    return;
  }

  blinkLed(200);
  blinkLed(200);
  blinkLed(200);
}

void operationState()
{
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);

  bool isConnected = connectToNetwork(g_ssid, g_password);

  if (isConnected)
  {
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    // WiFi.persist(true)
    wifiStatusLEDTurnOFF();
  }
  else
  {
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
    i++;
    blinkLed(500);

    if (i == 10)
    {
      return false;
    }
  }

  return true;
}

int sendDataToServer(const String& host, const String& message)
{
  WiFiClient client;
  HTTPClient http;

  http.begin(client, host);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("token", readToken());
  int httpResponseCode = http.POST(message);

  http.end();

  return httpResponseCode;
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

    if (!jsonObj.containsKey("ssid") || !jsonObj.containsKey("password") ||
        !jsonObj.containsKey("sm-air") || !jsonObj.containsKey("sm-water"))
    {
      AsyncWebServerResponse *response = request->beginResponse(400);
      request->send(response);
      return;
    }

    String ssid = jsonObj["ssid"];
    String pass = jsonObj["password"];

    if (ssid.length() == 0 || pass.length() == 0)
    {
      AsyncWebServerResponse *response = request->beginResponse(400);
      request->send(response);
      return;
    }

    g_ssid = ssid;
    g_password = pass;

    String airValue = jsonObj["sm-air"];
    String waterValue = jsonObj["sm-water"];
    String token = jsonObj["token"];

    writeToken(token);

    char buff[15];
    sprintf(buff, "#1!%s!%s#1", airValue.c_str(), waterValue.c_str());
    Serial.println(buff);

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

  server.on("/device-id", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", DEVICE_ID);
  });

  server.onNotFound(notFound);

  server.begin();

  pinMode(RESET_BUTTON_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

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
