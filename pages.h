const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en" dir="ltr"><head><meta charset="utf-8"><title>Plant Healthcheck Station Config</title></head><body style="display: flex;justify-content: center;flex-direction: column;align-content: center;align-items: center;">
<h1>Plant Healthcheck Station Config</h1>
<div style="display: flex;justify-content: center;align-items: baseline;flex-direction: column;">
<div style="display: flex;align-items: baseline;">
<b>Device ID: </b><p style="margin-top:0; margin-bottom:5px;" id="device-id">AlmafaMokus1</p>
</div>
<div style="display: flex;align-items: baseline;">
<b>Version: </b><p style="margin:0;">v0.0.1</p>
</div>
</div>
<p style="max-width: 400px;">You can configure the station's network conncection and the moisture sensor's base values. Please double check the WiFi SSID and the password before you finish the configuration process.</p>
<div style="display: flex;max-width: 300px;flex-direction: column;justify-content: center;align-items: center;">
<p>First of all, please enter the WiFi station name (SSID) and password below:</p>
<div style="display: flex;flex-direction: column;width: 100%;">
<p style="margin-bottom: 5px;">WiFi SSID</p>
<input type="text" id="ssid">
</div>
<div style="display: flex;flex-direction: column;width: 100%;">
<p style="margin-bottom: 5px;">WiFi password</p>
<input type="password" id="password" >
</div>
<div style="height: 1px;background: black;margin-top: 20px;margin-bottom: 20px;width: 100%;"></div>
<p style="margin-top: 0px;">Now you can configure the Soil Moisture sensor's base values. Every sensors are different a bit, so you have to configure the 100% (max) value and the 0% (min) value.</p>
<h2 id="sm-value-cap">0</h2>
<p style="margin-bottom: 0px;">Air Value: please clean the soil moisture sensor, wipe it off. Then write the value into the "Air value" box.</p>
<div style="display: flex;flex-direction: column;width: 100%;margin-bottom: 20px;">
<p style="margin-bottom: 5px;">Air value</p>
<input type="number" id="sm-air">
</div>
<p style="margin-bottom: 0px;">Water Value: please dip the bottom of the sensor into a glass of water. Then write the value into the "Air value" box.</p>
<div style="display: flex;flex-direction: column;width: 100%;margin-bottom: 20px;">
<p style="margin-bottom: 5px;">Water Value</p>
<input type="number" id="sm-water">
</div>
<button id="btn-finish" style="background: rgb(99 221 227);border: none;width: 100px;height: 50px;font-size: 16px;margin-top: 30px;cursor: pointer;">Finish</button>
</div>
<script>
  function fetchDeviceID() {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", "/device-id", true);
    xhr.onload = function (e) {
      if (xhr.readyState === 4) {
        if (xhr.status === 200) {
          document.getElementById("device-id").innerHTML =xhr.responseText;
        } else {
          console.error(xhr.statusText);
        }
      }
    };
    xhr.onerror = function (e) {
      console.error(xhr.statusText);
    };
    xhr.send(null);
  };

  fetchDeviceID();

  function fetchSoilMoisture() {
    const xhr = new XMLHttpRequest();
    xhr.open("GET", "/soil-moisture-value", true);
    xhr.onload = function (e) {
      if (xhr.readyState === 4) {
        if (xhr.status === 200) {
          document.getElementById("sm-value-cap").innerHTML =xhr.responseText;
        } else {
          console.error(xhr.statusText);
        }
      }
    };
    xhr.onerror = function (e) {
      console.error(xhr.statusText);
    };
    xhr.send(null);

    setTimeout(fetchSoilMoisture, 1000);
  };

  fetchSoilMoisture();

  var button = document.getElementById("btn-finish");
  button.addEventListener("click",function(e){
    var ssid = document.getElementById("ssid").value;
    var password = document.getElementById("password").value;
    var smAir = document.getElementById("sm-air").value;
    var smWater = document.getElementById("sm-water").value;
    
    var xmlhttp = new XMLHttpRequest();
    var theUrl = "/config";
    xmlhttp.open("POST", theUrl);
    
    xmlhttp.setRequestHeader("Content-Type", "application/json;charset=UTF-8");

    xmlhttp.onload = function (e) {
      if (xmlhttp.readyState === 4) {
        if (xmlhttp.status === 200) {
          location.href = "/done";
        } else {
          location.href = "/error";
        }
      }
    };
    xmlhttp.onerror = function (e) {
      console.error(xmlhttp.statusText);
    };
    xmlhttp.send(JSON.stringify({ "ssid": ssid, "password": password, "sm-air":smAir, "sm-water":smWater }));
  },false);
</script>
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
