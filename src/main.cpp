#include <Arduino.h>

#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <stdarg.h>
#define SERIAL_PRINTF_MAX_BUFF 256
#define F_PRECISION 6

#include "pages.h"

ESP8266WebServer server(80);

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

void serialPrintf(const char *fmt, ...);

String g_ssid = "";
String g_password = "";

void processSerialCommand(const String &cmd)
{
  char cmdChar = cmd[0];
  String message = "";

  if (cmd.length() > 2)
  {
    message = cmd.substring(2, cmd.length());
  }

  if (cmdChar == '0')
  {
    configState(message);
  }

  if (cmdChar == '1')
  {
    operationState();
  }

  if (cmdChar == '2')
  {
    sendDataToServer(message);
  }

  if (cmdChar == '3')
  {
    checkConnectionState();
  }

  if (cmdChar == '9')
  {
    idleState();
  }
}

void configState(const String &message)
{
  String stationSSID = message.substring(0, message.indexOf('!'));
  String stationPassword = message.substring(message.indexOf('!') + 1, message.length());

  if (stationSSID.length() == 0 || stationPassword.length() == 0)
  {
    Serial.println("Using default AP credentials");

    stationSSID = "IoT WiFi Station";
    stationPassword = "";
  }

  if (stationPassword == "*")
  {
    stationPassword = "";
  }

  WiFi.disconnect();
  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP(stationSSID, stationPassword);
  if (!result)
  {
    Serial.println("Failed!");

    serialPrintf("#AP_FAILED");
    return;
  }

  Serial.println("Ready");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();

  Serial.println("HTTP server started");

  serialPrintf("#AP_READY!%s", IP.toString().c_str());
}

void operationState()
{
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);

  bool isConnected = connectToNetwork(g_ssid, g_password);

  if (isConnected)
  {
    Serial.println('\n');
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

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
    Serial.print(++i);
    Serial.print(' ');

    if (i == 10)
    {

      return false;
    }
  }

  return true;
}

void checkConnectionState()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    serialPrintf("#WIFI_CONNECTED!%s", WiFi.localIP().toString().c_str());
  }
  else
  {
    serialPrintf("#WIFI_CONNECTION_FAILED");
  }
}

void idleState()
{
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();

  serialPrintf("#IDLE_STATE_READY");
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

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

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

    serialPrintf("#CONFIG_FAILED");
    return;
  }

  if (server.hasArg("sm-air") || server.hasArg("sm-water") || server.arg("sm-air") != NULL || server.arg("sm-water") != NULL)
  {

    serialPrintf("#CUSTOM_CFG!%s!%s", server.arg("sm-air").c_str(), server.arg("sm-water").c_str());
  }

  g_ssid = server.arg("ssid");
  g_password = server.arg("password");

  String s = DONE_page;
  server.send(200, "text/html", s);

  serialPrintf("#CONFIG_DONE");

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

/**
 * --------------------------------------------------------------
 * Perform simple printing of formatted data
 * Supported conversion specifiers:
 *      d, i     signed int
 *      u        unsigned int
 *      ld, li   signed long
 *      lu       unsigned long
 *      f        double
 *      c        char
 *      s        string
 *      %        '%'
 * Usage: %[conversion specifier]
 * Note: This function does not support these format specifiers:
 *      [flag][min width][precision][length modifier]
 * --------------------------------------------------------------
 */
void serialPrintf(const char *fmt, ...)
{
  /* buffer for storing the formatted data */
  char buf[SERIAL_PRINTF_MAX_BUFF];
  char *pbuf = buf;
  /* pointer to the variable arguments list */
  va_list pargs;

  /* Initialise pargs to point to the first optional argument */
  va_start(pargs, fmt);
  /* Iterate through the formatted string to replace all
  conversion specifiers with the respective values */
  while (*fmt)
  {
    if (*fmt == '%')
    {
      switch (*(++fmt))
      {
      case 'd':
      case 'i':
        pbuf += sprintf(pbuf, "%d", va_arg(pargs, int));
        break;
      case 'u':
        pbuf += sprintf(pbuf, "%u", va_arg(pargs, unsigned int));
        break;
      case 'l':
        switch (*(++fmt))
        {
        case 'd':
        case 'i':
          pbuf += sprintf(pbuf, "%ld", va_arg(pargs, long));
          break;
        case 'u':
          pbuf += sprintf(pbuf, "%lu",
                          va_arg(pargs, unsigned long));
          break;
        }
        break;
      case 'f':
        pbuf += strlen(dtostrf(va_arg(pargs, double),
                               1, F_PRECISION, pbuf));
        break;

      case 'c':
        *(pbuf++) = (char)va_arg(pargs, int);
        break;
      case 's':
        pbuf += sprintf(pbuf, "%s", va_arg(pargs, char *));
        break;
      case '%':
        *(pbuf++) = '%';
        break;
      default:
        break;
      }
    }
    else
    {
      *(pbuf++) = *fmt;
    }

    fmt++;
  }

  *pbuf = '\0';

  va_end(pargs);
  Serial.println(buf);
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

  serialPrintf("#MODULE_READY");
}

void loop()
{
  server.handleClient();

  String serialCmd = readFromSerialIfAvailable();

  if (serialCmd.length() > 0)
  {
    processSerialCommand(serialCmd);
  }
}
