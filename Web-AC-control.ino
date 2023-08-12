/* Copyright 2019 Motea Marius

  This example code will create a webserver that will provide basic control to AC units using the web application
  build with javascript/css. User config zone need to be updated if a different class than Collix need to be used.
  Javasctipt file may also require minor changes as in current version it will not allow to set fan speed if Auto mode
  is selected (required for Coolix).

*/
#if defined(ESP8266)
#include <LittleFS.h>
#else
#include <SPIFFS.h>
#endif

#ifndef FILESYSTEM
#ifdef ESP8266
#define FILESYSTEM LittleFS
#else
#define FILESYSTEM SPIFFS
#endif  // defined(ESP8266)
#endif

#if (FILESYSTEM == LittleFS)
#define FILESYSTEMSTR "LittleFS"
#else
#define FILESYSTEMSTR "SPIFFS"
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#endif  // ESP8266

#if defined(ESP32)
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>
#endif  // ESP32

#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
// OTA
#include <ArduinoOTA.h>  // only for InternalStorage
#include <ArduinoHttpClient.h>
// MQTT
#include <ArduinoMqttClient.h>

//// ###### User configuration space for AC library classes ##########

#include <ir_Fujitsu.h>  //  replace library based on your AC unit model, check https://github.com/crankyoldgit/IRremoteESP8266

// ==================== start of TUNEABLE PARAMETERS ====================

#if defined(ARDUINO_ESP8266_WEMOS_D1R1) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_ESP8266_WEMOS_D1MINILITE)

//4x IR LEDs emitter (940nm)
//1x IR receiver (38kHz)
//Configurable IO (Default: Sender - D3/GPIO0, Receiver - D4/GPIO2)
//https://wiki.wemos.cc/products:d1_mini_shields:ir_controller_shield

// The GPIO an IR detector/demodulator is connected to D4/GPIO2)
const uint16_t kRecvPin = 2;

// GPIO to use to control the IR LED circuit D3/GPIO0.
const uint16_t kIrLedPin = 0;

#else

// The GPIO an IR detector/demodulator is connected to. Recommended: 14 (D5)
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = 14;

// GPIO to use to control the IR LED circuit. Recommended: 4 (D2).
const uint16_t kIrLedPin = 4;

#endif

// ARRAH2E = 1,  ///< (1) AR-RAH2E, AR-RAC1E, AR-RAE1E, AR-RCE1E, AR-RAH2U,
//               ///<     AR-REG1U (Default)
//               ///< Warning: Use on incorrect models can cause the A/C to lock
//               ///< up, requring the A/C to be physically powered off to fix.
//               ///< e.g. AR-RAH1U may lock up with a Swing command.
// ARDB1,        ///< (2) AR-DB1, AR-DL10 (AR-DL10 swing doesn't work)
// ARREB1E,      ///< (3) AR-REB1E, AR-RAH1U (Similar to ARRAH2E but no horiz
//               ///<     control)
// ARJW2,        ///< (4) AR-JW2  (Same as ARDB1 but with horiz control)
// ARRY4,        ///< (5) AR-RY4 (Same as AR-RAH2E but with clean & filter)
// ARREW4E,      ///< (6) Similar to ARRAH2E, but with different temp config.

// The Serial connection baud rate.
// NOTE: Make sure you set your Serial Monitor to the same speed.
//const uint32_t kBaudRate = 115200;

// As this program is a special purpose capture/resender, let's use a larger
// than expected buffer so we can handle very large IR messages.
const uint16_t kCaptureBufferSize = 1024;  // 1024 == ~511 bits

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
const uint8_t kTimeout = 50;  // Milli-Seconds

// kFrequency is the modulation frequency all UNKNOWN messages will be sent at.
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

// ==================== end of TUNEABLE PARAMETERS ====================

// OTA
const short VERSION = 2;

// MQTT
WiFiClient wifiClientMQTT;
MqttClient mqttClient(wifiClientMQTT);

// The IR transmitter.
IRFujitsuAC irsend(kIrLedPin);
// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
// Somewhere to store the captured message.
decode_results results;

/// ##### End user configuration ######

struct state {
  uint8_t temperature = 22, fan = 0, operation = 0;
  bool swingvert;
  bool swinghor;
  bool powerStatus;
  bool econo;
  bool lowNoise;
  bool heatTenC;
};

// Configuration that we'll store on disk
struct DeviceSettings {
  char deviceName[32];
  char wifiPass[32];
  unsigned short wifiChannel;
  bool startAP;
  bool hideSSID;
  bool enableIRRecv;
  // Sync
  bool synchronise;
  bool syncMe;
  bool innerDoor;
  bool outerDoor;
  // OTA
  bool autoupdate;
  char updSvr[32];
  unsigned short updSvrPort;
  // MQTT
  bool useMQTT;
  char mqtt_broker[32];
  unsigned short mqtt_port;
  char mqtt_topic[32];
  char mqtt_username[32];
  char mqtt_password[32];
  // Model
  unsigned short irModel;
};

File fsUploadFile;

// core
state acState;
DeviceSettings deviceSettings;
const char* filenameS = "/deviceSettings.json";

#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;
#endif  // ESP8266
#if defined(ESP32)
WebServer server(80);
#endif  // ESP32

unsigned long wifiPreviousMillis = 0;
unsigned long wifiReconnectInterval = 30000;

unsigned long mqttPreviousMillis = 0;
unsigned long mqttReconnectInterval = 30000;

// MQTT SetUP
void setMQTT() {

  if (!deviceSettings.useMQTT) {
    if (mqttClient.connected()) {
      mqttClient.stop();
    }
    return;
  }

  // You can provide a unique client ID, if not set the library uses Arduin-millis()
  // Each client must have a unique client ID
  mqttClient.setId(String(WiFi.macAddress()));

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(deviceSettings.mqtt_username, deviceSettings.mqtt_password);

  // By default the library connects with the "clean session" flag set,
  // you can disable this behaviour by using
  // mqttClient.setCleanSession(false);

  //Serial.print("Attempting to connect to the MQTT broker: ");
  //Serial.println(deviceSettings.mqtt_broker);

  if (!mqttClient.connect(deviceSettings.mqtt_broker, deviceSettings.mqtt_port)) {
    //Serial.print("MQTT connection failed! Error code = ");
    //Serial.println(mqttClient.connectError());

    return;
  }

  //Serial.println("You're connected to the MQTT broker!");
  //Serial.println();

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  //Serial.print("Subscribing to topic: ");
  //Serial.println(deviceSettings.mqtt_topic);
  //Serial.println();

  // subscribe to a topic
  // the second parameter set's the QoS of the subscription,
  // the the library supports subscribing at QoS 0, 1, or 2
  int subscribeQos = 1;

  mqttClient.subscribe(deviceSettings.mqtt_topic, subscribeQos);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);

  //Serial.print("Waiting for messages on topic: ");
  //Serial.println(deviceSettings.mqtt_topic);
  //Serial.println();

  // send state on connect
  DynamicJsonDocument root(1024);
  root["version"] = VERSION;
  root["deviceName"] = deviceSettings.deviceName;
  root["mode"] = acState.operation;
  root["fan"] = acState.fan;
  root["temp"] = acState.temperature;
  root["power"] = acState.powerStatus;
  root["swingvert"] = acState.swingvert;
  root["swinghor"] = acState.swinghor;
  root["econo"] = acState.econo;
  root["lowNoise"] = acState.lowNoise;
  root["heatTenC"] = acState.heatTenC;

  String output;
  serializeJson(root, output);
  sendMQTT("{\"islocal\":true,\"ep\":\"state\", \"message\":" + output + "}");
}

// MQTT callback
void onMqttMessage(int messageSize) {

  // we received a message, print out the topic and contents
  //Serial.print("Received a message with topic '");
  //Serial.print(mqttClient.messageTopic());
  //Serial.print("', duplicate = ");
  //Serial.print(mqttClient.messageDup() ? "true" : "false");
  //Serial.print(", QoS = ");
  //Serial.print(mqttClient.messageQoS());
  //Serial.print(", retained = ");
  //Serial.print(mqttClient.messageRetain() ? "true" : "false");
  //Serial.print("', length ");
  //Serial.print(messageSize);
  //Serial.println(" bytes:");

  String newMessage;
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    //Serial.print((char)mqttClient.read());
    newMessage = newMessage + (char)mqttClient.read();  //Conver *byte to String
  }
  //Serial.println(newMessage);
  //Serial.println();
  //Serial.println();

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, newMessage);
  if (error) {
    //Serial.println("Failed to read message.");
    //return;
  }
  if (root.containsKey("ep") && root.containsKey("message")) {

    String ep;
    serializeJson(root["ep"], ep);
    if (root["ep"] == "turbo") {
      if (root["message"]["turbo"]) {
        root["message"]["econo"] = false;
      }
    }
    String messageState;
    serializeJson(root["message"], messageState);

    //Serial.print("Endpoint: ");Serial.println(ep);
    //Serial.print("Message: ");Serial.println(messageState);
    //Serial.println();

    if (root.containsKey("islocal")) {
      return;
    }

    bool mustSend;
    if (root["ep"] == "state") {

      if (root["message"].containsKey("temp")) {
        acState.temperature = (uint8_t)root["message"]["temp"];
      }
      if (root["message"].containsKey("fan")) {
        acState.fan = (uint8_t)root["message"]["fan"];
      }
      if (root["message"].containsKey("power")) {
        acState.powerStatus = root["message"]["power"];
      }
      if (root["message"].containsKey("mode")) {
        acState.operation = root["message"]["mode"];
      }
      if (root["message"].containsKey("swingvert")) {
        acState.swingvert = root["message"]["swingvert"];
      }
      if (root["message"].containsKey("swinghor")) {
        acState.swinghor = root["message"]["swinghor"];
      }
      if (root["message"].containsKey("econo")) {
        acState.econo = root["message"]["econo"];
      }
      if (root["message"].containsKey("lowNoise")) {
        acState.lowNoise = root["message"]["lowNoise"];
      }
      if (root["message"].containsKey("heatTenC")) {
        acState.heatTenC = root["message"]["heatTenC"];
      }

      if (!root.containsKey("rst")) {

        if (acState.powerStatus) {

          //Serial.println("Power On / Change params");
          //Serial.print("operation: ");Serial.println(acState.operation);
          //Serial.print("fan: ");Serial.println(acState.fan);
          //Serial.print("temperature: ");Serial.println(acState.temperature);
          //Serial.print("swingvert: ");Serial.println(acState.swingvert);
          //Serial.print("swinghor: ");Serial.println(acState.swinghor);

          irsend.setTemp(acState.temperature);

          if (acState.operation == 0) {
            irsend.setMode(kFujitsuAcModeAuto);
          } else if (acState.operation == 1) {
            irsend.setMode(kFujitsuAcModeCool);
          } else if (acState.operation == 2) {
            irsend.setMode(kFujitsuAcModeDry);
          } else if (acState.operation == 3) {
            irsend.setMode(kFujitsuAcModeFan);
          } else if (acState.operation == 4) {
            irsend.setMode(kFujitsuAcModeHeat);
          }

          if (acState.fan == 0) {
            irsend.setFanSpeed(kFujitsuAcFanAuto);
          } else if (acState.fan == 1) {
            irsend.setFanSpeed(kFujitsuAcFanQuiet);
          } else if (acState.fan == 2) {
            irsend.setFanSpeed(kFujitsuAcFanLow);
          } else if (acState.fan == 3) {
            irsend.setFanSpeed(kFujitsuAcFanMed);
          } else if (acState.fan == 4) {
            irsend.setFanSpeed(kFujitsuAcFanHigh);
          }

          if ((acState.swingvert) && (acState.swinghor)) {
            irsend.setSwing(kFujitsuAcSwingBoth);
          } else {
            if (acState.swingvert) {
              irsend.setSwing(kFujitsuAcSwingVert);
            } else if (acState.swinghor) {
              irsend.setSwing(kFujitsuAcSwingHoriz);
            } else {
              irsend.setSwing(kFujitsuAcSwingOff);
            }
          }

          irsend.setCmd(kFujitsuAcCmdTurnOn);

        } else {

          //Serial.println("Power Off");
          irsend.setCmd(kFujitsuAcCmdTurnOff);
        }

        mustSend = true;
      }

    } else if (root["ep"] == "turbo") {

      if (root["message"]["turbo"]) {

        //Serial.println("Powerful mode On");
        irsend.setCmd(kFujitsuAcCmdPowerful);
        mustSend = true;
        acState.econo = false;
      }

    } else if (root["ep"] == "stepv") {

      if (root["message"].containsKey("swingvert")) {
        acState.swingvert = root["message"]["swingvert"];
      }

      if (root["message"]["stepv"]) {

        //Serial.println("Step vertical");
        irsend.setCmd(kFujitsuAcCmdStepVert);
        mustSend = true;
      }

    } else if (root["ep"] == "steph") {

      if (root["message"].containsKey("swinghor")) {
        acState.swinghor = root["message"]["swinghor"];
      }

      if (root["message"]["steph"]) {

        //Serial.println("Step horizontal");
        irsend.setCmd(kFujitsuAcCmdStepHoriz);
        mustSend = true;
      }

    } else if (root["ep"] == "econo") {

      if (root["message"].containsKey("econo")) {
        acState.econo = root["message"]["econo"];
      }

      //Serial.print("Economy: ");Serial.println(acState.econo);
      irsend.setCmd(kFujitsuAcCmdEcono);
      mustSend = true;

    } else if (root["ep"] == "lnoise") {

      if (root["message"].containsKey("lowNoise")) {
        acState.lowNoise = root["message"]["lowNoise"];
      }

      //Serial.println("Outdoor Unit Low Noise");
      //irsend.setCmd(kFujitsuAcOutsideQuietOffset);
      if (acState.lowNoise) {
        irsend.setOutsideQuiet(true);
      } else {
        irsend.setOutsideQuiet(false);
      }
      mustSend = true;

    } else if (root["ep"] == "heattenc") {

      if (root["message"].containsKey("heatTenC")) {
        acState.heatTenC = root["message"]["heatTenC"];
      }

      //Serial.println("Set 10C Heat");
      if (acState.heatTenC) {
        irsend.set10CHeat(true);
      } else {
        irsend.set10CHeat(false);
      }
      mustSend = true;
    }

    if (mustSend) {
      disableEnableIRRecvAndSend();
      //sendMQTT("{\"islocal\":true,\"ep\":\"" + ep + "\", \"message\":" + messageState + "}");
      syncWithOtherDevices(messageState, "/" + ep);
    }
  }
  /*
  } else if (root.containsKey("restart")){
    
    ESP.restart();
    
  } else if (root.containsKey("getState")){
  */
  bool mustRestart = root.containsKey("restart");

  root.clear();
  //DynamicJsonDocument root(1024);
  root["version"] = VERSION;
  root["deviceName"] = deviceSettings.deviceName;
  root["mode"] = acState.operation;
  root["fan"] = acState.fan;
  root["temp"] = acState.temperature;
  root["power"] = acState.powerStatus;
  root["swingvert"] = acState.swingvert;
  root["swinghor"] = acState.swinghor;
  root["econo"] = acState.econo;
  root["lowNoise"] = acState.lowNoise;
  root["heatTenC"] = acState.heatTenC;

  String output;
  serializeJson(root, output);

  if (mustRestart) {
    sendMQTT("{\"rst\":true,\"ep\":\"state\", \"message\":" + output + "}");
    delay(300);
    ESP.restart();
  } else {
    sendMQTT("{\"islocal\":true,\"ep\":\"state\", \"message\":" + output + "}");
  }
}

// MQTT sending message to topic
void sendMQTT(String message) {

  if (!deviceSettings.useMQTT) {
    return;
  }

  //Serial.print("Sending message to topic: ");Serial.println(deviceSettings.mqtt_topic);
  //Serial.println(message);

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(deviceSettings.mqtt_topic, true);
  mqttClient.print(message);
  mqttClient.endMessage();

  //Serial.println();
}

void reconnectWiFi() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - wifiPreviousMillis >= wifiReconnectInterval)) {
    //Serial.print(millis());Serial.print(" - ");Serial.println("Reconnecting to WiFi...");
    //WiFi.disconnect(false);
    WiFi.reconnect();
    wifiPreviousMillis = currentMillis;
  }
}

void reconnectMQTT() {

  unsigned long mqttCurrentMillis = millis();

  if (deviceSettings.useMQTT) {

    if ((!mqttClient.connected()) && (mqttCurrentMillis - mqttPreviousMillis >= mqttReconnectInterval)) {

      mqttClient.stop();
      delay(1000);
      setMQTT();
      /*
      Serial.print(millis());Serial.print(" - ");Serial.println("Reconnecting to MQTT Broker...");
      
      if (!mqttClient.connect(deviceSettings.mqtt_broker, deviceSettings.mqtt_port)) {
        
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());
        mqttClient.stop();
        delay(1000);
      }
      */
      mqttPreviousMillis = mqttCurrentMillis;
    }

    mqttClient.poll();
  }
}

void handleSketchDownload(bool forceupdate) {

  if ((!deviceSettings.autoupdate) && (!forceupdate)) {
    return;
  }

  const unsigned long CHECK_INTERVAL = 60000;
  static unsigned long previousMillis;
  unsigned long currentMillis = millis();

  if (!forceupdate) {

    if (currentMillis - previousMillis < CHECK_INTERVAL)
      return;
    previousMillis = currentMillis;
  }

  const char* SERVER = deviceSettings.updSvr;                    //"192.168.9.1"; // must be string for HttpClient
  const unsigned short SERVER_PORT = deviceSettings.updSvrPort;  //80;
#if defined(ARDUINO_ESP8266_WEMOS_D1R1) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_ESP8266_WEMOS_D1MINILITE)
  const char* PATH = "/acwifictrl/ota/wemos-v%d.bin";
#else
  const char* PATH = "/acwifictrl/ota/nodemcu-v%d.bin";
#endif

  WiFiClient wifiClient;
  HttpClient client(wifiClient, SERVER, SERVER_PORT);

  char buff[32];
  snprintf(buff, sizeof(buff), PATH, VERSION + 1);

  //Serial.print("Check for update file ");
  //Serial.println(buff);

  client.get(buff);

  int statusCode = client.responseStatusCode();
  //Serial.print("Update status code: ");
  //Serial.println(statusCode);
  if (statusCode != 200) {
    if (forceupdate) {
      server.send(statusCode, "text/html", "Manual update");
      delay(100);
    }
    client.stop();
    return;
  }

  long length = client.contentLength();
  if (length == HttpClient::kNoContentLengthHeader) {
    client.stop();
    //Serial.println("Server didn't provide Content-length header. Can't continue with update.");
    if (forceupdate) {
      server.send(200, "text/html", "Server didn't provide Content-length header. Can't continue with update.");
      delay(100);
    }
    return;
  }
  //Serial.print("Server returned update file of size ");
  //Serial.print(length);
  //Serial.println(" bytes");

  if (!InternalStorage.open(length)) {
    client.stop();
    //Serial.println("There is not enough space to store the update. Can't continue with update.");
    if (forceupdate) {
      server.send(200, "text/html", "There is not enough space to store the update. Can't continue with update.");
      delay(100);
    }
    return;
  }

  byte b;
  while (length > 0) {
    if (!client.readBytes(&b, 1))  // reading a byte with timeout
      break;
    InternalStorage.write(b);
    length--;
  }
  InternalStorage.close();
  client.stop();
  if (length > 0) {
    //Serial.print("Timeout downloading update file at ");
    //Serial.print(length);
    //Serial.println(" bytes. Can't continue with update.");
    if (forceupdate) {
      server.send(200, "text/html", "Timeout downloading update file. Can't continue with update.");
      delay(100);
    }
    return;
  }
  if (forceupdate) {
    server.send(200, "text/html", "Sketch update apply and reset.");
    delay(100);
  }

  //Serial.println("Sketch update apply and reset.");
  //Serial.flush();
  ESP.restart();
}

bool handleFileRead(String path) {
  //  send the right file to the client (if it exists)
  // Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";
  // If a folder is requested, send the index file
  String contentType = getContentType(path);
  // Get the MIME type
  String pathWithGz = path + ".gz";
  if (FILESYSTEM.exists(pathWithGz) || FILESYSTEM.exists(path)) {
    // If the file exists, either as a compressed archive, or normal
    // If there's a compressed version available
    if (FILESYSTEM.exists(pathWithGz))
      path += ".gz";  // Use the compressed verion
    File file = FILESYSTEM.open(path, "r");
    //  Open the file
    server.streamFile(file, contentType);
    //  Send it to the client
    file.close();
    // Close the file again
    // Serial.println(String("\tSent file: ") + path);
    return true;
  }
  // Serial.println(String("\tFile Not Found: ") + path);
  // If the file doesn't exist, return false
  return false;
}

String getContentType(String filename) {
  // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".svgz")) return "image/svg+xml";
  return "text/plain";
}

void handleFileUpload() {  // upload a new file to the FILESYSTEM
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    //Serial.print("handleFileUpload Name: ");
    //Serial.println(filename);
    fsUploadFile = FILESYSTEM.open(filename, "w");
    // Open the file for writing in FILESYSTEM (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
    // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      // If the file was successfully created
      fsUploadFile.close();
      // Close the file again
      //Serial.print("handleFileUpload Size: ");
      //Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");
      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Serving servForceupdate
void servForceupdate() {
  //server.send(200, "text/html", "forceupdate");
  //delay(100);
  handleSketchDownload(true);
}

// Serving restart
void servRestart() {
  
  if (deviceSettings.useMQTT) {
    DynamicJsonDocument root(1024);
    root["version"] = VERSION;
    root["deviceName"] = deviceSettings.deviceName;
    root["mode"] = acState.operation;
    root["fan"] = acState.fan;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["swingvert"] = acState.swingvert;
    root["swinghor"] = acState.swinghor;
    root["econo"] = acState.econo;
    root["lowNoise"] = acState.lowNoise;
    root["heatTenC"] = acState.heatTenC;

    String output;
    serializeJson(root, output);

    sendMQTT("{\"rst\":true,\"ep\":\"state\", \"message\":" + output + "}");
  }
  
  server.send(200, "text/html", "reset");
  delay(200);
  ESP.restart();

}

// Format filesystem
void servFFS() {
  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("key")) {
       if ((root["key"] == 56)){
         bool result = FILESYSTEM.format();
         if (result) {
           server.send(200, "application/json", "{\"result\":true}");
           } else {
             server.send(400, "application/json", "{\"result\":false}");
             }
       } else {
        server.send(404, "application/json", "{\"result\":false,\"error\":\"FAIL. Missing arguments.\"}");
       }
    } else {
      server.send(404, "application/json", "{\"result\":false,\"error\":\"FAIL. Missing arguments.\"}");
    }
  }
}

// Serving getDevState
void getDevState() {

  DynamicJsonDocument root(1024);

  root["version"] = VERSION;
  root["deviceName"] = deviceSettings.deviceName;
  root["mode"] = acState.operation;
  root["fan"] = acState.fan;
  root["temp"] = acState.temperature;
  root["power"] = acState.powerStatus;
  root["swingvert"] = acState.swingvert;
  root["swinghor"] = acState.swinghor;
  root["econo"] = acState.econo;
  root["lowNoise"] = acState.lowNoise;
  root["heatTenC"] = acState.heatTenC;

  String output;
  serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", output);
}

// Serving getDeviceSettings
void getDeviceSettings() {

  //  if (FILESYSTEM.exists(filenameS)) {
  File file = FILESYSTEM.open(filenameS, "r");
  //  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, file);
  if (error) {
    //Serial.println("Failed to read file, using default configuration");
  }

  root["version"] = VERSION;
  root["deviceName"] = root["deviceName"] | deviceSettings.deviceName;
  root["wifiPass"] = root["wifiPass"] | deviceSettings.wifiPass;
  root["wifiChannel"] = root["wifiChannel"] | deviceSettings.wifiChannel;
  root["startAP"] = root["startAP"] | deviceSettings.startAP;
  root["hideSSID"] = root["hideSSID"] | deviceSettings.hideSSID;
  root["enableIRRecv"] = root["enableIRRecv"] | deviceSettings.enableIRRecv;
  // Sync
  root["synchronise"] = root["synchronise"] | deviceSettings.synchronise;
  root["syncMe"] = root["syncMe"] | deviceSettings.syncMe;
  root["innerDoor"] = root["innerDoor"] | deviceSettings.innerDoor;
  root["outerDoor"] = root["outerDoor"] | deviceSettings.outerDoor;
  // OTA
  root["autoupdate"] = root["autoupdate"] | deviceSettings.autoupdate;
  root["updSvr"] = root["updSvr"] | deviceSettings.updSvr;
  root["updSvrPort"] = root["updSvrPort"] | deviceSettings.updSvrPort;
  // MQTT
  root["useMQTT"] = root["useMQTT"] | deviceSettings.useMQTT;
  root["mqtt_broker"] = root["mqtt_broker"] | deviceSettings.mqtt_broker;
  root["mqtt_port"] = root["mqtt_port"] | deviceSettings.mqtt_port;
  root["mqtt_topic"] = root["mqtt_topic"] | deviceSettings.mqtt_topic;
  root["mqtt_username"] = root["mqtt_username"] | deviceSettings.mqtt_username;
  root["mqtt_password"] = root["mqtt_password"] | deviceSettings.mqtt_password;
  // Model
  root["irModel"] = root["irModel"] | deviceSettings.irModel;

  // Close the file (File's destructor doesn't close the file)
  file.close();

  String output;
  serializeJson(root, output);
  server.send(200, "application/json", output);
}

// Serving getSyncMe
void getSyncMe() {

  if (deviceSettings.syncMe) {
    server.send(200, "text/plain", "true");
  } else {
    server.send(200, "text/plain", "false");
  }
}

// Serving postState
void postState() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("temp")) {
      acState.temperature = (uint8_t)root["temp"];
    }
    if (root.containsKey("fan")) {
      acState.fan = (uint8_t)root["fan"];
    }
    if (root.containsKey("power")) {
      acState.powerStatus = root["power"];
    }
    if (root.containsKey("mode")) {
      acState.operation = root["mode"];
    }
    if (root.containsKey("swingvert")) {
      acState.swingvert = root["swingvert"];
    }
    if (root.containsKey("swinghor")) {
      acState.swinghor = root["swinghor"];
    }
    if (root.containsKey("econo")) {
      acState.econo = root["econo"];
    }
    if (root.containsKey("lowNoise")) {
      acState.lowNoise = root["lowNoise"];
    }
    if (root.containsKey("heatTenC")) {
      acState.heatTenC = root["heatTenC"];
    }

    root["version"] = VERSION;
    root["deviceName"] = deviceSettings.deviceName;
    root["mode"] = acState.operation;
    root["fan"] = acState.fan;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["swingvert"] = acState.swingvert;
    root["swinghor"] = acState.swinghor;
    root["econo"] = acState.econo;
    root["lowNoise"] = acState.lowNoise;
    root["heatTenC"] = acState.heatTenC;

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    if (acState.powerStatus) {

      //Serial.println("Power On / Change params");
      //Serial.print("operation: ");Serial.println(acState.operation);
      //Serial.print("fan: ");Serial.println(acState.fan);
      //Serial.print("temperature: ");Serial.println(acState.temperature);
      //Serial.print("swingvert: ");Serial.println(acState.swingvert);
      //Serial.print("swinghor: ");Serial.println(acState.swinghor);

      irsend.setTemp(acState.temperature);

      if (acState.operation == 0) {
        irsend.setMode(kFujitsuAcModeAuto);
      } else if (acState.operation == 1) {
        irsend.setMode(kFujitsuAcModeCool);
      } else if (acState.operation == 2) {
        irsend.setMode(kFujitsuAcModeDry);
      } else if (acState.operation == 3) {
        irsend.setMode(kFujitsuAcModeFan);
      } else if (acState.operation == 4) {
        irsend.setMode(kFujitsuAcModeHeat);
      }

      if (acState.fan == 0) {
        irsend.setFanSpeed(kFujitsuAcFanAuto);
      } else if (acState.fan == 1) {
        irsend.setFanSpeed(kFujitsuAcFanQuiet);
      } else if (acState.fan == 2) {
        irsend.setFanSpeed(kFujitsuAcFanLow);
      } else if (acState.fan == 3) {
        irsend.setFanSpeed(kFujitsuAcFanMed);
      } else if (acState.fan == 4) {
        irsend.setFanSpeed(kFujitsuAcFanHigh);
      }

      if ((acState.swingvert) && (acState.swinghor)) {
        irsend.setSwing(kFujitsuAcSwingBoth);
      } else {
        if (acState.swingvert) {
          irsend.setSwing(kFujitsuAcSwingVert);
        } else if (acState.swinghor) {
          irsend.setSwing(kFujitsuAcSwingHoriz);
        } else {
          irsend.setSwing(kFujitsuAcSwingOff);
        }
      }

      irsend.setCmd(kFujitsuAcCmdTurnOn);

    } else {

      //Serial.println("Power Off");

      irsend.setCmd(kFujitsuAcCmdTurnOff);
    }

    disableEnableIRRecvAndSend();

    sendMQTT("{\"islocal\":true,\"ep\":\"state\", \"message\":" + output + "}");

    syncWithOtherDevices(output, "/state");
  }
}

// Serving postTurbo
void postTurbo() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root["turbo"]) {
      
      acState.econo = false;

      root["version"] = VERSION;
      root["deviceName"] = deviceSettings.deviceName;
      root["mode"] = acState.operation;
      root["fan"] = acState.fan;
      root["temp"] = acState.temperature;
      root["power"] = acState.powerStatus;
      root["swingvert"] = acState.swingvert;
      root["swinghor"] = acState.swinghor;
      root["econo"] = acState.econo;
      root["lowNoise"] = acState.lowNoise;
      root["heatTenC"] = acState.heatTenC;
    }

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    if (root["turbo"]) {

      //Serial.println("Powerful mode On");

      irsend.setCmd(kFujitsuAcCmdPowerful);

      disableEnableIRRecvAndSend();

      sendMQTT("{\"islocal\":true,\"ep\":\"turbo\", \"message\":" + output + "}");

      syncWithOtherDevices(output, "/turbo");
    }
  }
}

// Serving postStepV
void postStepV() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("swingvert")) {
      acState.swingvert = root["swingvert"];
    }

    if (root["stepv"]) {

      root["version"] = VERSION;
      root["deviceName"] = deviceSettings.deviceName;
      root["mode"] = acState.operation;
      root["fan"] = acState.fan;
      root["temp"] = acState.temperature;
      root["power"] = acState.powerStatus;
      root["swingvert"] = acState.swingvert;
      root["swinghor"] = acState.swinghor;
      root["econo"] = acState.econo;
      root["lowNoise"] = acState.lowNoise;
      root["heatTenC"] = acState.heatTenC;
    }

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    if (root["stepv"]) {

      //Serial.println("Step vertical");

      irsend.setCmd(kFujitsuAcCmdStepVert);

      disableEnableIRRecvAndSend();

      sendMQTT("{\"islocal\":true,\"ep\":\"stepv\", \"message\":" + output + "}");

      syncWithOtherDevices(output, "/stepv");
    }
  }
}

// Serving postStepH
void postStepH() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("swinghor")) {
      acState.swinghor = root["swinghor"];
    }

    if (root["steph"]) {

      root["version"] = VERSION;
      root["deviceName"] = deviceSettings.deviceName;
      root["mode"] = acState.operation;
      root["fan"] = acState.fan;
      root["temp"] = acState.temperature;
      root["power"] = acState.powerStatus;
      root["swingvert"] = acState.swingvert;
      root["swinghor"] = acState.swinghor;
      root["econo"] = acState.econo;
      root["lowNoise"] = acState.lowNoise;
      root["heatTenC"] = acState.heatTenC;
    }

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    if (root["steph"]) {

      //Serial.println("Step horizontal");

      irsend.setCmd(kFujitsuAcCmdStepHoriz);

      disableEnableIRRecvAndSend();

      sendMQTT("{\"islocal\":true,\"ep\":\"steph\", \"message\":" + output + "}");

      syncWithOtherDevices(output, "/steph");
    }
  }
}

// Serving postEcono
void postEcono() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("econo")) {
      acState.econo = root["econo"];
    }

    root["version"] = VERSION;
    root["deviceName"] = deviceSettings.deviceName;
    root["mode"] = acState.operation;
    root["fan"] = acState.fan;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["swingvert"] = acState.swingvert;
    root["swinghor"] = acState.swinghor;
    root["econo"] = acState.econo;
    root["lowNoise"] = acState.lowNoise;
    root["heatTenC"] = acState.heatTenC;

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    //Serial.print("Economy: ");Serial.println(acState.econo);
    irsend.setCmd(kFujitsuAcCmdEcono);

    disableEnableIRRecvAndSend();

    sendMQTT("{\"islocal\":true,\"ep\":\"econo\", \"message\":" + output + "}");

    syncWithOtherDevices(output, "/econo");
  }
}

// Serving postLNoise
void postLNoise() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("lowNoise")) {
      acState.lowNoise = root["lowNoise"];
    }

    root["version"] = VERSION;
    root["deviceName"] = deviceSettings.deviceName;
    root["mode"] = acState.operation;
    root["fan"] = acState.fan;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["swingvert"] = acState.swingvert;
    root["swinghor"] = acState.swinghor;
    root["econo"] = acState.econo;
    root["lowNoise"] = acState.lowNoise;
    root["heatTenC"] = acState.heatTenC;

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    //Serial.println("Outdoor Unit Low Noise");
    //irsend.setCmd(kFujitsuAcOutsideQuietOffset);
    if (acState.lowNoise) {
      irsend.setOutsideQuiet(true);
    } else {
      irsend.setOutsideQuiet(false);
    }

    disableEnableIRRecvAndSend();

    sendMQTT("{\"islocal\":true,\"ep\":\"lnoise\", \"message\":" + output + "}");

    syncWithOtherDevices(output, "/lnoise");
  }
}

// Serving postHeatTenC
void postHeatTenC() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root.containsKey("heatTenC")) {
      acState.heatTenC = root["heatTenC"];
    }

    root["version"] = VERSION;
    root["deviceName"] = deviceSettings.deviceName;
    root["mode"] = acState.operation;
    root["fan"] = acState.fan;
    root["temp"] = acState.temperature;
    root["power"] = acState.powerStatus;
    root["swingvert"] = acState.swingvert;
    root["swinghor"] = acState.swinghor;
    root["econo"] = acState.econo;
    root["lowNoise"] = acState.lowNoise;
    root["heatTenC"] = acState.heatTenC;

    String output;
    serializeJson(root, output);
    //server.send(200, "text/plain", output);
    server.send(200, "application/json", output);

    delay(200);

    //Serial.println("Set 10C Heat");

    if (acState.heatTenC) {
      irsend.set10CHeat(true);
    } else {
      irsend.set10CHeat(false);
    }

    disableEnableIRRecvAndSend();

    sendMQTT("{\"islocal\":true,\"ep\":\"heattenc\", \"message\":" + output + "}");

    syncWithOtherDevices(output, "/heattenc");
  }
}

// Serving postDeviceSettings
void postDeviceSettings() {

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    File file = FILESYSTEM.open(filenameS, "w");
    bool newSettingsAP = false;
    bool changeStateMQTT = false;
    bool newIRmodel = false;

    if (root.containsKey("deviceName")) {
      newSettingsAP = deviceSettings.deviceName != root["deviceName"];
      strlcpy(deviceSettings.deviceName, root["deviceName"], sizeof(deviceSettings.deviceName));
    }
    if (root.containsKey("wifiPass")) {
      newSettingsAP = deviceSettings.wifiPass != root["wifiPass"] || newSettingsAP;
      strlcpy(deviceSettings.wifiPass, root["wifiPass"], sizeof(deviceSettings.wifiPass));
    }
    if (root.containsKey("wifiChannel")) {
      newSettingsAP = deviceSettings.wifiChannel != root["wifiChannel"] || newSettingsAP;
      deviceSettings.wifiChannel = root["wifiChannel"];
    }
    if (root.containsKey("startAP")) {
      newSettingsAP = deviceSettings.startAP != root["startAP"] || newSettingsAP;
      deviceSettings.startAP = root["startAP"];
    }
    if (root.containsKey("hideSSID")) {
      newSettingsAP = deviceSettings.hideSSID != root["hideSSID"] || newSettingsAP;
      deviceSettings.hideSSID = root["hideSSID"];
    }
    if (root.containsKey("enableIRRecv")) {
      if (deviceSettings.enableIRRecv && root["enableIRRecv"] == false) {
        irrecv.disableIRIn();
      } else if (!deviceSettings.enableIRRecv && root["enableIRRecv"] == true) {
        irrecv.enableIRIn();
      }
      deviceSettings.enableIRRecv = root["enableIRRecv"];
    }
    if (newSettingsAP && deviceSettings.startAP) {
      WiFi.softAP(deviceSettings.deviceName, deviceSettings.wifiPass, deviceSettings.wifiChannel, deviceSettings.hideSSID);
      //Serial.println("Soft AP started");
    } else if (newSettingsAP && !deviceSettings.startAP) {
      if (WiFi.softAPdisconnect(true)) {
        //Serial.println("Soft AP stopped");
      } else {
        WiFi.softAP(deviceSettings.deviceName, deviceSettings.wifiPass, deviceSettings.wifiChannel, deviceSettings.hideSSID);
        //Serial.println("Can't stop Soft AP)");
        //Serial.println("Soft AP started");
      }
    }
    // Sync
    if (root.containsKey("synchronise")) {
      deviceSettings.synchronise = root["synchronise"];
    }
    if (root.containsKey("syncMe")) {
      deviceSettings.syncMe = root["syncMe"];
    }
    if (root.containsKey("innerDoor")) {
      deviceSettings.innerDoor = root["innerDoor"];
    }
    if (root.containsKey("outerDoor")) {
      deviceSettings.outerDoor = root["outerDoor"];
    }
    // OTA
    if (root.containsKey("autoupdate")) {
      deviceSettings.autoupdate = root["autoupdate"];
    }
    if (root.containsKey("updSvr")) {
      strlcpy(deviceSettings.updSvr, root["updSvr"], sizeof(deviceSettings.updSvr));
    }
    if (root.containsKey("updSvrPort")) {
      deviceSettings.updSvrPort = root["updSvrPort"];
    }
    // MQTT
    if (root.containsKey("useMQTT")) {
      changeStateMQTT = deviceSettings.useMQTT != root["useMQTT"];
      deviceSettings.useMQTT = root["useMQTT"];
    }
    if (root.containsKey("mqtt_broker")) {
      changeStateMQTT = deviceSettings.mqtt_broker != root["mqtt_broker"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_broker, root["mqtt_broker"], sizeof(deviceSettings.mqtt_broker));
    }
    if (root.containsKey("mqtt_port")) {
      changeStateMQTT = deviceSettings.mqtt_port != root["mqtt_port"] || changeStateMQTT;
      deviceSettings.mqtt_port = root["mqtt_port"];
    }
    if (root.containsKey("mqtt_topic")) {
      changeStateMQTT = deviceSettings.mqtt_topic != root["mqtt_topic"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_topic, root["mqtt_topic"], sizeof(deviceSettings.mqtt_topic));
    }
    if (root.containsKey("mqtt_username")) {
      changeStateMQTT = deviceSettings.mqtt_username != root["mqtt_username"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_username, root["mqtt_username"], sizeof(deviceSettings.mqtt_username));
    }
    if (root.containsKey("mqtt_password")) {
      changeStateMQTT = deviceSettings.mqtt_password != root["mqtt_password"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_password, root["mqtt_password"], sizeof(deviceSettings.mqtt_password));
    }
    // Model
    if (root.containsKey("irModel")) {
      newIRmodel = deviceSettings.irModel != root["irModel"];
      deviceSettings.irModel = root["irModel"];
    }

    if (newIRmodel) {
      // Set up what we want to send. See ir_Fujitsu.cpp for all the options.
      // See `fujitsu_ac_remote_model_t` in `ir_Fujitsu.h` for a list of models.
      if (deviceSettings.irModel == 1) {
        irsend.setModel(ARRAH2E);
      } else if (deviceSettings.irModel == 2) {
        irsend.setModel(ARDB1);
      } else if (deviceSettings.irModel == 3) {
        irsend.setModel(ARREB1E);
      } else if (deviceSettings.irModel == 4) {
        irsend.setModel(ARJW2);
      } else if (deviceSettings.irModel == 5) {
        irsend.setModel(ARRY4);
      } else if (deviceSettings.irModel == 6) {
        irsend.setModel(ARREW4E);
      }
    }

    // Serialize JSON to file
    serializeJson(root, file);

    // Close the file (File's destructor doesn't close the file)
    file.close();

    String output;
    serializeJson(root, output);
    server.send(200, "application/json", output);

    delay(200);

    //Serial.println("Device Settings:");
    //Serial.print("    ");
    //Serial.println(output);

    if (changeStateMQTT) {
      setMQTT();
    }
  }
}

void setDeviceSettings() {

  File file;
  // read settings
  bool fileExists = FILESYSTEM.exists(filenameS);
  if (fileExists) {
    file = FILESYSTEM.open(filenameS, "r");
  } else {
    file = FILESYSTEM.open(filenameS, "w");
  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, file);
  if (error) {
    //Serial.println("Failed to read file, using default configuration");
  }

  strlcpy(deviceSettings.deviceName, root["deviceName"] | "AC Remote Control", sizeof(deviceSettings.deviceName));
  strlcpy(deviceSettings.wifiPass, root["wifiPass"] | "testpass", sizeof(deviceSettings.wifiPass));
  deviceSettings.wifiChannel = root["wifiChannel"] | 1;
  deviceSettings.startAP = root["startAP"] | false;
  deviceSettings.hideSSID = root["hideSSID"] | false;
  deviceSettings.enableIRRecv = root["enableIRRecv"] | false;
  // Sync
  deviceSettings.synchronise = root["synchronise"] | false;
  deviceSettings.syncMe = root["syncMe"] | false;
  deviceSettings.innerDoor = root["innerDoor"] | false;
  deviceSettings.outerDoor = root["outerDoor"] | false;
  // OTA
  deviceSettings.autoupdate = root["autoupdate"] | false;
  strlcpy(deviceSettings.updSvr, root["updSvr"] | "192.168.9.1", sizeof(deviceSettings.updSvr));
  deviceSettings.updSvrPort = root["updSvrPort"] | 80;
  // MQTT
  deviceSettings.useMQTT = root["useMQTT"] | false;
  strlcpy(deviceSettings.mqtt_broker, root["mqtt_broker"] | "", sizeof(deviceSettings.mqtt_broker));
  deviceSettings.mqtt_port = root["mqtt_port"] | 0;
  strlcpy(deviceSettings.mqtt_topic, root["mqtt_topic"] | "", sizeof(deviceSettings.mqtt_topic));
  strlcpy(deviceSettings.mqtt_username, root["mqtt_username"] | "", sizeof(deviceSettings.mqtt_username));
  strlcpy(deviceSettings.mqtt_password, root["mqtt_password"] | "", sizeof(deviceSettings.mqtt_password));
  // Model
  deviceSettings.irModel = root["irModel"] | 1;

  if (!fileExists) {

    root["deviceName"] = deviceSettings.deviceName;
    root["wifiPass"] = deviceSettings.wifiPass;
    root["wifiChannel"] = deviceSettings.wifiChannel;
    root["startAP"] = deviceSettings.startAP;
    root["hideSSID"] = deviceSettings.hideSSID;
    root["enableIRRecv"] = deviceSettings.enableIRRecv;
    // Sync
    root["synchronise"] = deviceSettings.synchronise;
    root["syncMe"] = deviceSettings.syncMe;
    root["innerDoor"] = deviceSettings.innerDoor;
    root["outerDoor"] = deviceSettings.outerDoor;
    // OTA
    root["autoupdate"] = deviceSettings.autoupdate;
    root["updSvr"] = deviceSettings.updSvr;
    root["updSvrPort"] = deviceSettings.updSvrPort;
    // MQTT
    root["useMQTT"] = deviceSettings.useMQTT;
    root["mqtt_broker"] = deviceSettings.mqtt_broker;
    root["mqtt_port"] = deviceSettings.mqtt_port;
    root["mqtt_topic"] = deviceSettings.mqtt_topic;
    root["mqtt_username"] = deviceSettings.mqtt_username;
    root["mqtt_password"] = deviceSettings.mqtt_password;
    // Model
    root["irModel"] = deviceSettings.irModel;

    // Serialize JSON to file
    serializeJson(root, file);
  }

  // Close the file (File's destructor doesn't close the file)
  file.close();

  //Serial.print("Setting: ");
  //Serial.println(deviceSettings);
}

void syncWithOtherDevices(String output, String endPoint) {

  if (deviceSettings.synchronise) {

    // TODO deviceSettings.syncDevs array

    const char* syncSERVER = "192.168.2.21";    //deviceSettings.syncSvr;// must be string for HttpClient
    const unsigned short syncSERVER_PORT = 80;  //deviceSettings.syncSvrPort;

    WiFiClient wifiClient;
    HttpClient client(wifiClient, syncSERVER, syncSERVER_PORT);

    client.get("/syncMe");

    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    //Serial.print("Status code GET: ");
    //Serial.println(statusCode);
    //Serial.print("Response GET: ");
    //Serial.println(response);

    if ((statusCode == 200) && (response == "true")) {

      client.post(endPoint, "application/json", output);

      // // read the status code and body of the response
      // int statusCode = client.responseStatusCode();
      // String response = client.responseBody();
      
      // Serial.print("Status code: ");
      // Serial.println(statusCode);
      // Serial.print("Response: ");
      // Serial.println(response);
    }

    //Serial.println("Wait five seconds");
    //delay(5000);
  }
}

void setWebServer() {

#if defined(ESP8266)
  httpUpdateServer.setup(&server);
#endif  // ESP8266

  server.on("/", []() {
    server.sendHeader("Location", String("ui.html"), true);
    server.send(302, "text/plain", "");
  });


  server.on("/state", HTTP_GET, getDevState);
  server.on("/state", HTTP_POST, postState);
  server.on("/stepv", HTTP_POST, postStepV);
  server.on("/steph", HTTP_POST, postStepH);
  server.on("/turbo", HTTP_POST, postTurbo);
  server.on("/econo", HTTP_POST, postEcono);
  server.on("/lnoise", HTTP_POST, postLNoise);
  server.on("/heattenc", HTTP_POST, postHeatTenC);
  server.on("/settings", HTTP_GET, getDeviceSettings);
  server.on("/settings", HTTP_POST, postDeviceSettings);
  server.on("/syncMe", HTTP_GET, getSyncMe);
  server.on("/reset", servRestart);
  server.on("/formatfs", HTTP_POST, servFFS);
  server.on("/forceupdate", servForceupdate);
  server.on(
    "/file-upload", HTTP_POST,
    // if the client posts to the upload page
    []() {
      // Send status 200 (OK) to tell the client we are ready to receive
      server.send(200);
    },
    handleFileUpload);  // Receive and save the file

  server.on("/file-upload", HTTP_GET, []() {
    // if the client requests the upload page

    String html = "<form method=\"post\" enctype=\"multipart/form-data\">";
    html += "<input type=\"file\" name=\"name\">";
    html += "<input class=\"button\" type=\"submit\" value=\"Upload\">";
    html += "</form>";
    server.send(200, "text/html", html);
  });

  server.serveStatic("/", FILESYSTEM, "/", "max-age=86400");

  server.onNotFound(handleNotFound);
  server.begin();
}

void disableEnableIRRecvAndSend() {

  if (deviceSettings.enableIRRecv) {

    irrecv.disableIRIn();
    irsend.send();
    irrecv.enableIRIn();

  } else {
    irsend.send();
  }
}

// IR Resv
void printIRresults() {

  if (!deviceSettings.enableIRRecv){
    return;
    }
/*
  // Check if an IR message has been received.
  if (irrecv.decode(&results)) {  // We have captured something.
    // The capture has stopped at this point.
    decode_type_t protocol = results.decode_type;
    uint16_t size = results.bits;
    bool success = true;
    // Is it a protocol we don't understand?
    if (protocol == decode_type_t::UNKNOWN) {  // Yes.
      // Convert the results into an array suitable for sendRaw().
      // resultToRawArray() allocates the memory we need for the array.
      uint16_t* raw_array = resultToRawArray(&results);
      // Find out how many elements are in the array.
      size = getCorrectedRawLength(&results);
#if SEND_RAW
      // Send it out via the IR LED circuit.
      //irsend.sendRaw(raw_array, size, kFrequency);
      //Serial.println("send raw");
      //Serial.println(resultToHumanReadableBasic(&results));
      //Serial.println(resultToSourceCode(&results));
#endif  // SEND_RAW
      // Deallocate the memory allocated by resultToRawArray().
      delete[] raw_array;
    } else if (hasACState(protocol)) {  // Does the message require a state[]?
      // It does, so send with bytes instead.
      //success = irsend.send(protocol, results.state, size / 8);
      //Serial.println("message require a state[]");
      //Serial.println(resultToHumanReadableBasic(&results));
      for (int i = 0; i < sizeof(results.state); i++) {
        //Serial.print(results.state[i], HEX);
        //Serial.println();
      }
      //Serial.println(resultToSourceCode(&results));

    } else {  // Anything else must be a simple message protocol. ie. <= 64 bits
      //success = irsend.send(protocol, results.value, size);
      //Serial.println("must be a simple message protocol. ie. <= 64 bits");
      //Serial.println(resultToHumanReadableBasic(&results));
      //Serial.println(resultToSourceCode(&results));
    }
    // Resume capturing IR messages. It was not restarted until after we sent
    // the message so we didn't capture our own message.
    */
    irrecv.resume();
    /*
    // Display a crude timestamp & notification.
    uint32_t now = millis();
    
    Serial.printf(
        "%06u.%03u: A %d-bit %s message was %ssuccessfully retransmitted.\n",
        now / 1000, now % 1000, size, typeToString(protocol).c_str(),
        success ? "" : "un");
    
  }*/
  //yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.
}

void setup() {

  //Serial.begin(kBaudRate, SERIAL_8N1);
  //while (!Serial)  // Wait for the serial connection to be establised.
  //  delay(50);
  //Serial.println();
  //Serial.print("Sketch version ");
  //Serial.println(VERSION);

  //Serial.print("IR input Pin ");
  //Serial.println(kRecvPin);
  //Serial.print("IR transmit Pin ");
  //Serial.println(kIrLedPin);

  //Serial.println("mounting " FILESYSTEMSTR "...");

  if (!FILESYSTEM.begin()) {
    //Serial.println("Failed to mount file system");
    return;
  }

  setDeviceSettings();

  if (deviceSettings.enableIRRecv) {
    irrecv.enableIRIn();  // Start up the IR receiver.
  }
  irsend.begin();  // Start up the IR sender.
  delay(200);
  // Set up what we want to send. See ir_Fujitsu.cpp for all the options.
  // See `fujitsu_ac_remote_model_t` in `ir_Fujitsu.h` for a list of models.
  if (deviceSettings.irModel == 1) {
    irsend.setModel(ARRAH2E);
  } else if (deviceSettings.irModel == 2) {
    irsend.setModel(ARDB1);
  } else if (deviceSettings.irModel == 3) {
    irsend.setModel(ARREB1E);
  } else if (deviceSettings.irModel == 4) {
    irsend.setModel(ARJW2);
  } else if (deviceSettings.irModel == 5) {
    irsend.setModel(ARRY4);
  } else if (deviceSettings.irModel == 6) {
    irsend.setModel(ARREW4E);
  }


  delay(500);

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  //wifiManager.setConnectTimeout(2);
  if (!wifiManager.autoConnect(deviceSettings.deviceName, deviceSettings.wifiPass)) {
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  WiFi.softAP(deviceSettings.deviceName, deviceSettings.wifiPass, deviceSettings.wifiChannel, deviceSettings.hideSSID);

  if (!deviceSettings.startAP) {
    if (WiFi.softAPdisconnect(true)) {
      //Serial.println("Soft AP stopped");
    } else {
      WiFi.softAP(deviceSettings.deviceName, deviceSettings.wifiPass, deviceSettings.wifiChannel, deviceSettings.hideSSID);
      //Serial.println("Can't stop Soft AP");
      //Serial.println("Soft AP started");
    }
  }

  // MQTT
  setMQTT();

  // WEB SERVER
  setWebServer();
}

void loop() {

  reconnectWiFi();

  // check for updates
  handleSketchDownload(false);

  // MQTT
  reconnectMQTT();

  // print from IR
  printIRresults();

  server.handleClient();
}
