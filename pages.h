const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en" dir="ltr"><head><meta charset="utf-8"><title>Plant Healthcheck Station Config</title></head><body style="display: flex;justify-content: center;flex-direction: column;align-content: center;align-items: center;">
<h1>Plant Healthcheck Station Config</h1>
<p style="max-width: 400px;">You can configure the station's network conncection and the moisture sensor's base values. Please double check the WiFi SSID and the password before you finish the configuration process.</p>
<form method="post" action="/config" style="display: flex;max-width: 300px;flex-direction: column;justify-content: center;align-items: center;">
<p>First of all, please enter the WiFi station name (SSID) and password below:</p>
<div style="display: flex;flex-direction: column;width: 100%;">
<p style="margin-bottom: 5px;">WiFi SSID</p>
<input type="text" name="ssid">
</div>
<div style="display: flex;flex-direction: column;width: 100%;">
<p style="margin-bottom: 5px;">WiFi password</p>
<input type="password" name="password" >
</div>
<div style="height: 1px;background: black;margin-top: 20px;margin-bottom: 20px;width: 100%;"></div>
<p style="margin-top: 0px;">Now you can configure the Soil Moisture sensor's base values. Every sensors are different a bit, so you have to configure the 100% (max) value and the 0% (min) value.</p>
<p style="margin-bottom: 0px;">Air Value: please clean the soil moisture sensor, wipe it off. Then write the value into the "Air value" box.</p>
<div style="display: flex;flex-direction: column;width: 100%;margin-bottom: 20px;">
<p style="margin-bottom: 5px;">Air value</p>
<input type="text" name="sm-air">
</div>
<p style="margin-bottom: 0px;">Water Value: please dip the bottom of the sensor into a glass of water. Then write the value into the "Air value" box.</p>
<div style="display: flex;flex-direction: column;width: 100%;margin-bottom: 20px;">
<p style="margin-bottom: 5px;">Water Value</p>
<input type="text" name="sm-water">
</div>
<input type="submit" value="Finish" style="background: rgb(99 221 227);border: none;width: 100px;height: 50px;font-size: 16px;margin-top: 30px;cursor: pointer;">
</form>
</body>
</html>
)=====";

const char DONE_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" dir="ltr">
<head>
<meta charset="utf-8">
<title>Plant Healthcheck Station Config</title>
</head>
<body style="display: flex;justify-content: center;flex-direction: column;align-content: center;align-items: center;">
<h1>Plant Healthcheck Station Config</h1>
<div style="width: 100px;height: 100px;display: flex;justify-content: center;align-items: center;border-radius: 100px;background: rgb(99 221 227);">
<p>Ok</p>
</div>
<p>Configuration finished</p>
<p style="max-width: 400px;">The station's AP will disappear and the Plant Healthcheck Station will connect to the given WiFi access point. If you would like to configure the station again, please RESET the Plant Healthcheck Station.</p>
</body>
</html>
)=====";

const char ERROR_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" dir="ltr">
<head>
<meta charset="utf-8">
<title>Plant Healthcheck Station Config</title>
</head>
<body style="display: flex;justify-content: center;flex-direction: column;align-content: center;align-items: center;">
<h1>Plant Healthcheck Station Config</h1>
<div style="width: 100px;height: 100px;display: flex;justify-content: center;align-items: center;border-radius: 100px;background: red;">
<p>Error</p>
</div>
<p>Something failed during the configuration</p>
<p style="max-width: 400px;">Please start the configuration again, reset the station.</p>
</body>
</html>
)=====";

const char NOT_FOUND_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en" dir="ltr">
<head>
<meta charset="utf-8">
<title>Plant Healthcheck Station Config</title>
</head>
<body style="display: flex;justify-content: center;flex-direction: column;align-content: center;align-items: center;">
<h1>Plant Healthcheck Station Config</h1>
<div style="width: 100px;height: 100px;display: flex;justify-content: center;align-items: center;border-radius: 100px;background: red;">
<p>404</p>
</div>
<p>Not found</p>
</body>
</html>
)=====";
