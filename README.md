# Web-AC-control

This is just a web interface for the IRremoteESP8266 library.  

![Web Gui Preview](https://github.com/DAtanassov/Web-AC-control/blob/main/printscreen.png)  

## Instructions:

 - Connect IR led to one GPIO pin (recommended pin is for D1 mini D3/GPIO0, for nodeMCU D2/GPIO4)

 - Edit Web-AC-control.ino header marked as "User space". You will need to import the library dedicated for your AC model. Every library has its own commands for AC mode and fan speed that will need to be replace according to commands available in .h file of the library.

 - Flash the firmware in ESP board using 1M or 2M of SPIFFS storage.

 - Connect the board to your wifi network (look for "AC Remote Control" SSID and follow WiFi Manager wizard)

 - Upload web application files in SPIFFS storage using build in web form located at /file-upload path.
 
 
## REST API:

Browser console will show the ajax calls to ESP8266 board. Running configuration can be displayed with GET request to /state path, any value can be changed with http PUT request to same path.

Ex:

```
➜  ~ curl 192.168.2.22/state
{"mode":4,"fan":0,"temp":24,"power":true,"swingvert":true,"econo":false,"lowNoise":true,"heatTenC":false}%
➜  ~ curl -X PUT -d '{"temp":22}' 192.168.2.22/state
{"temp":22}%
➜  ~ curl 192.168.2.22/state
{"mode":4,"fan":0,"temp":22,"power":true,"swingvert":true,"econo":false,"lowNoise":true,"heatTenC":false}%
```

## DEBUG:

Use mobile phone camera to see if the led is sending any IR signals when buttons are pressed. This will show if the circuit was properly made and the selected GPIO pin is the correct one.


## Credits:

Idea: https://github.com/mariusmotea/esp8266-AC-control 