/*  This file is part of WEB-AC-CONTROL project */

#ifndef __WEB_AC_CONTROL_H
#define __WEB_AC_CONTROL_H

#include <LittleFS.h>
#ifndef FILESYSTEM
  #define FILESYSTEM LittleFS
#endif
//#define FILESYSTEMSTR "LittleFS"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

//#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

// IR
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// OTA
#include <ArduinoOTA.h>  // only for InternalStorage
#include <ArduinoHttpClient.h>

// MQTT
#include <ArduinoMqttClient.h>

// TIMERS
#include <SimpleTimer.h>

//// ###### User configuration space for AC library classes ##########

#include <ir_Fujitsu.h>  //  replace library based on your AC unit model, check https://github.com/crankyoldgit/IRremoteESP8266

// OTA
const short VERSION = 3;

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
const uint32_t kBaudRate = 115200;

// As this program is a special purpose capture/resender, let's use a larger
// than expected buffer so we can handle very large IR messages.
const uint16_t kCaptureBufferSize = 1024;  // 1024 == ~511 bits

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
const uint8_t kTimeout = 50;  // Milli-Seconds

// kFrequency is the modulation frequency all UNKNOWN messages will be sent at.
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

// ==================== end of TUNEABLE PARAMETERS ====================

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

//Device state in memory.
// Device state is not stored on disk, but can be updated by an mqtt server.
struct DeviceState {
  uint8_t temperature = 22, fan = 0, operation = 0;
  bool swingvert;
  bool swinghor;
  bool powerStatus;
  bool econo;
  bool lowNoise;
  bool heatTenC;
} acState;

// Configuration that we'll store on disk
struct DeviceSettings {
  char deviceName[32];
  char wifiPass[32];
  unsigned short wifiChannel;
  bool startAP;
  bool hideSSID;
  // Sync
  bool synchronize;
  bool syncMe;
  bool enableIRRecv;
  // OTA
  bool otaAutoUpd;
  char otaURL[32];
  unsigned short otaURLPort;
  char otaPath[32];
  // MQTT
  bool useMQTT;
  char mqtt_broker[32];
  unsigned short mqtt_port;
  char mqtt_topic[32];
  char mqtt_username[32];
  char mqtt_password[32];
  // Model
  unsigned short irModel;
} deviceSettings;

File fsUploadFile;

const char* filenameS = "/deviceSettings.json";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdateServer;

// TIMERS
SimpleTimer timerWiFi;
SimpleTimer timerMQTT;
SimpleTimer timerOTA;

#endif