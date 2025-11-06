/*  This file is part of WEB-AC-CONTROL project */

#include "Web-AC-control.h"

/*############################## Settings ##############################*/

String GetStateString() {

  JsonDocument root;

  root["version"] = VERSION;
  root["deviceName"] = deviceSettings.deviceName;
  root["irModel"] = deviceSettings.irModel;
  root["mode"] = acState.operation;
  root["fan"] = acState.fan;
  root["temp"] = acState.temperature;
  root["power"] = acState.powerStatus;
  root["swingvert"] = acState.swingvert;
  root["swinghor"] = acState.swinghor;
  root["econo"] = acState.econo;
  root["lowNoise"] = acState.lowNoise;
  root["heatTenC"] = acState.heatTenC;

  String outputString;
  serializeJson(root, outputString);

  return outputString;
}

String GetSettingsString() {

  File file = FILESYSTEM.open(filenameS, "r");

  JsonDocument root;
  deserializeJson(root, file);
  /*
  DeserializationError error = deserializeJson(root, file);
  if (error) {
    Serial.println("Failed to read file, using default configuration");
  }
  */

  root["version"] = VERSION;
  root["deviceName"] = root["deviceName"] | deviceSettings.deviceName;
  root["wifiPass"] = root["wifiPass"] | deviceSettings.wifiPass;
  root["wifiChannel"] = root["wifiChannel"] | deviceSettings.wifiChannel;
  root["startAP"] = root["startAP"] | deviceSettings.startAP;
  root["hideSSID"] = root["hideSSID"] | deviceSettings.hideSSID;
  // Sync
  root["synchronize"] = root["synchronize"] | deviceSettings.synchronize;
  root["syncMe"] = root["syncMe"] | deviceSettings.syncMe;
  root["enableIRRecv"] = root["enableIRRecv"] | deviceSettings.enableIRRecv;
  // OTA
  root["otaAutoUpd"] = root["otaAutoUpd"] | deviceSettings.otaAutoUpd;
  root["otaURL"] = root["otaURL"] | deviceSettings.otaURL;
  root["otaURLPort"] = root["otaURLPort"] | deviceSettings.otaURLPort;
  root["otaPath"] = root["otaPath"] | deviceSettings.otaPath;
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

  String outputString;
  serializeJson(root, outputString);

  return outputString;
}

void ReadDeviceSettings() {

  File file;
  // read settings
  bool fileExists = FILESYSTEM.exists(filenameS);
  if (fileExists) {
    file = FILESYSTEM.open(filenameS, "r");
  } else {
    file = FILESYSTEM.open(filenameS, "w");
  }

  delay(500);
  JsonDocument root;
  if (fileExists) {

    DeserializationError error = deserializeJson(root, file);
    if (error) {
      //Serial.println("Failed to read file.");
    }
    delay(500);
  }

  strlcpy(deviceSettings.deviceName, root["deviceName"] | "AC Remote Control", sizeof(deviceSettings.deviceName));
  strlcpy(deviceSettings.wifiPass, root["wifiPass"] | "testpass", sizeof(deviceSettings.wifiPass));
  deviceSettings.wifiChannel = root["wifiChannel"] | 1;
  deviceSettings.startAP = root["startAP"] | false;
  deviceSettings.hideSSID = root["hideSSID"] | false;
  // Sync
  deviceSettings.synchronize = root["synchronize"] | false;
  deviceSettings.syncMe = root["syncMe"] | false;
  deviceSettings.enableIRRecv = root["enableIRRecv"] | false;
  // OTA
  deviceSettings.otaAutoUpd = root["otaAutoUpd"] | false;
  strlcpy(deviceSettings.otaURL, root["otaURL"] | "192.168.9.1", sizeof(deviceSettings.otaURL));
  deviceSettings.otaURLPort = root["otaURLPort"] | 80;
  strlcpy(deviceSettings.otaPath, root["otaPath"] | "update.json", sizeof(deviceSettings.otaPath));
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
    // Sync
    root["synchronize"] = deviceSettings.synchronize;
    root["syncMe"] = deviceSettings.syncMe;
    root["enableIRRecv"] = deviceSettings.enableIRRecv;
    // OTA
    root["otaAutoUpd"] = deviceSettings.otaAutoUpd;
    root["otaURL"] = deviceSettings.otaURL;
    root["otaURLPort"] = deviceSettings.otaURLPort;
    root["otaPath"] = deviceSettings.otaPath;
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

  delay(500);
  //Serial.print("Setting: ");
  //Serial.println(deviceSettings);
}

String SetDeviceSettings(String settings) {

  JsonDocument root;
  DeserializationError error = deserializeJson(root, settings);

  if (error) {
    return "404";
  } else {

    File file = FILESYSTEM.open(filenameS, "w");

    bool newSettingsAP = false;
    bool changeStateMQTT = false;
    bool newIRmodel = false;

    if (!root["deviceName"].isNull()) {
      newSettingsAP = deviceSettings.deviceName != root["deviceName"];
      strlcpy(deviceSettings.deviceName, root["deviceName"], sizeof(deviceSettings.deviceName));
    }
    if (!root["wifiPass"].isNull()) {
      newSettingsAP = deviceSettings.wifiPass != root["wifiPass"] || newSettingsAP;
      strlcpy(deviceSettings.wifiPass, root["wifiPass"], sizeof(deviceSettings.wifiPass));
    }
    if (!root["wifiChannel"].isNull()) {
      newSettingsAP = deviceSettings.wifiChannel != root["wifiChannel"] || newSettingsAP;
      deviceSettings.wifiChannel = root["wifiChannel"];
    }
    if (!root["startAP"].isNull()) {
      newSettingsAP = deviceSettings.startAP != root["startAP"] || newSettingsAP;
      deviceSettings.startAP = root["startAP"];
    }
    if (!root["hideSSID"].isNull()) {
      newSettingsAP = deviceSettings.hideSSID != root["hideSSID"] || newSettingsAP;
      deviceSettings.hideSSID = root["hideSSID"];
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
    if (!root["synchronize"].isNull()) {
      deviceSettings.synchronize = root["synchronize"];
    }
    if (!root["syncMe"].isNull()) {
      deviceSettings.syncMe = root["syncMe"];
    }
    if (!root["enableIRRecv"].isNull()) {
      if (deviceSettings.enableIRRecv && root["enableIRRecv"] == false) {
        irrecv.disableIRIn();
      } else if (!deviceSettings.enableIRRecv && root["enableIRRecv"] == true) {
        irrecv.enableIRIn();
      }
      deviceSettings.enableIRRecv = root["enableIRRecv"];
    }
    // OTA
    if (!root["otaAutoUpd"].isNull()) {
      deviceSettings.otaAutoUpd = root["otaAutoUpd"];
    }
    if (!root["otaURL"].isNull()) {
      strlcpy(deviceSettings.otaURL, root["otaURL"], sizeof(deviceSettings.otaURL));
    }
    if (!root["otaURLPort"].isNull()) {
      deviceSettings.otaURLPort = root["otaURLPort"];
    }
    if (!root["otaPath"].isNull()) {
      strlcpy(deviceSettings.otaPath, root["otaPath"], sizeof(deviceSettings.otaPath));
    }
    // MQTT
    if (!root["useMQTT"].isNull()) {
      changeStateMQTT = deviceSettings.useMQTT != root["useMQTT"];
      deviceSettings.useMQTT = root["useMQTT"];
    }
    if (!root["mqtt_broker"].isNull()) {
      changeStateMQTT = deviceSettings.mqtt_broker != root["mqtt_broker"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_broker, root["mqtt_broker"], sizeof(deviceSettings.mqtt_broker));
    }
    if (!root["mqtt_port"].isNull()) {
      changeStateMQTT = deviceSettings.mqtt_port != root["mqtt_port"] || changeStateMQTT;
      deviceSettings.mqtt_port = root["mqtt_port"];
    }
    if (!root["mqtt_topic"].isNull()) {
      changeStateMQTT = deviceSettings.mqtt_topic != root["mqtt_topic"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_topic, root["mqtt_topic"], sizeof(deviceSettings.mqtt_topic));
    }
    if (!root["mqtt_username"].isNull()) {
      changeStateMQTT = deviceSettings.mqtt_username != root["mqtt_username"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_username, root["mqtt_username"], sizeof(deviceSettings.mqtt_username));
    }
    if (!root["mqtt_password"].isNull()) {
      changeStateMQTT = deviceSettings.mqtt_password != root["mqtt_password"] || changeStateMQTT;
      strlcpy(deviceSettings.mqtt_password, root["mqtt_password"], sizeof(deviceSettings.mqtt_password));
    }
    // Model
    if (!root["irModel"].isNull()) {
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

    //Serial.println("Device Settings:");
    //Serial.print("    ");
    //Serial.println(output);

    if (changeStateMQTT) {
      setMQTT();
    }

    String output;
    serializeJson(root, output);
    return output;
  }
}

/*############################## Settings ##############################*/

/*################################ MQTT ################################*/

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
  sendMQTT("{\"islocal\":true,\"ep\":\"state\", \"message\":" + GetStateString() + "}");
}

// MQTT callback
void onMqttMessage(int messageSize) {

  // we received a message, print out the topic and contents
  /*
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', duplicate = ");
  Serial.print(mqttClient.messageDup() ? "true" : "false");
  Serial.print(", QoS = ");
  Serial.print(mqttClient.messageQoS());
  Serial.print(", retained = ");
  Serial.print(mqttClient.messageRetain() ? "true" : "false");
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  */

  String newMessage;
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    //Serial.print((char)mqttClient.read());
    newMessage = newMessage + (char)mqttClient.read();  //Conver *byte to String
  }
  //Serial.println(newMessage);
  //Serial.println();
  //Serial.println();

  JsonDocument root;
  deserializeJson(root, newMessage);
  /*
  DeserializationError error = deserializeJson(root, newMessage);
  if (error) {
    Serial.println("Failed to read message.");
    return;
  }
  */
  if (!root["ep"].isNull() && !root["message"].isNull()) {

    String ep;
    serializeJson(root["ep"], ep);
    if (root["ep"] == "turbo") {
      if (root["message"]["turbo"]) {
        root["message"]["econo"] = false;
      }
    }
    String messageState;
    serializeJson(root["message"], messageState);

    /*
    Serial.print("Endpoint: ");Serial.println(ep);
    Serial.print("Message: ");Serial.println(messageState);
    Serial.println();
    */

    if (!root["islocal"].isNull()) {
      return;
    }

    bool mustSend;
    if (root["ep"] == "state") {

      if (!root["message"]["temp"].isNull()) {
        acState.temperature = (uint8_t)root["message"]["temp"];
      }
      if (!root["message"]["fan"].isNull()) {
        acState.fan = (uint8_t)root["message"]["fan"];
      }
      if (!root["message"]["power"].isNull()) {
        acState.powerStatus = root["message"]["power"];
      }
      if (!root["message"]["mode"].isNull()) {
        acState.operation = root["message"]["mode"];
      }
      if (!root["message"]["swingvert"].isNull()) {
        acState.swingvert = root["message"]["swingvert"];
      }
      if (!root["message"]["swinghor"].isNull()) {
        acState.swinghor = root["message"]["swinghor"];
      }
      if (!root["message"]["econo"].isNull()) {
        acState.econo = root["message"]["econo"];
      }
      if (!root["message"]["lowNoise"].isNull()) {
        acState.lowNoise = root["message"]["lowNoise"];
      }
      if (!root["message"]["heatTenC"].isNull()) {
        acState.heatTenC = root["message"]["heatTenC"];
      }

      if (root["rst"].isNull()) {

        if (acState.powerStatus) {

          /*
          Serial.println("Power On / Change params");
          Serial.print("operation: ");Serial.println(acState.operation);
          Serial.print("fan: ");Serial.println(acState.fan);
          Serial.print("temperature: ");Serial.println(acState.temperature);
          Serial.print("swingvert: ");Serial.println(acState.swingvert);
          Serial.print("swinghor: ");Serial.println(acState.swinghor);
          */

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

      if (!root["message"]["swingvert"].isNull()) {
        acState.swingvert = root["message"]["swingvert"];
      }

      if (root["message"]["stepv"]) {

        //Serial.println("Step vertical");
        irsend.setCmd(kFujitsuAcCmdStepVert);
        mustSend = true;
      }

    } else if (root["ep"] == "steph") {

      if (!root["message"]["swinghor"].isNull()) {
        acState.swinghor = root["message"]["swinghor"];
      }

      if (root["message"]["steph"]) {

        //Serial.println("Step horizontal");
        irsend.setCmd(kFujitsuAcCmdStepHoriz);
        mustSend = true;
      }

    } else if (root["ep"] == "econo") {

      if (!root["message"]["econo"].isNull()) {
        acState.econo = root["message"]["econo"];
      }

      //Serial.print("Economy: ");Serial.println(acState.econo);
      irsend.setCmd(kFujitsuAcCmdEcono);
      mustSend = true;

    } else if (root["ep"] == "lnoise") {

      if (!root["message"]["lowNoise"].isNull()) {
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

      if (!root["message"]["heatTenC"].isNull()) {
        acState.heatTenC = root["message"]["heatTenC"];
      }

      //Serial.println("Set 10C Heat");
      if (acState.heatTenC) {
        irsend.set10CHeat(true);
      } else {
        irsend.set10CHeat(false);
      }
      mustSend = true;
    } else if (root["ep"] == "settings") {
      String newSettings;
      serializeJson(root["message"], newSettings);
      String output = SetDeviceSettings(newSettings);
      mustSend = false;
    }

    if (mustSend) {
      SendIR();
      //sendMQTT("{\"islocal\":true,\"ep\":\"" + ep + "\", \"message\":" + messageState + "}");
      syncOtherDevices(messageState, "/" + ep);
    }
  }
  /*
  } else if (!root["restart"].isNull()){
    
    ESP.restart();
    
  } else if (!root["getState"].isNull()){
  */
  bool mustRestart = !root["restart"].isNull();

  String output = GetStateString();

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

void SendIR() {

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

  if (!deviceSettings.enableIRRecv) {
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

/*################################ MQTT ################################*/

/*################################ OTA  ################################*/

void handleSketchDownload(bool forceupdate) {

  if ((!deviceSettings.otaAutoUpd) && (!forceupdate)) {
    return;
  }

  if (timerOTA.isReady() || forceupdate) {  // Check is ready a second timer

    if (!checkforupdate()) {
      server.send(200, "text/html", "No update available.");
      return;
    }

    const char* SERVER = deviceSettings.otaURL;
    const unsigned short SERVER_PORT = deviceSettings.otaURLPort;
    const char* PATH = deviceSettings.otaPath;

    WiFiClient wifiClient;
    HttpClient client(wifiClient, SERVER, SERVER_PORT);

    client.get(PATH);

    int statusCode = client.responseStatusCode();
    //Serial.print("Update status code: ");
    //Serial.println(statusCode);
    if (statusCode != 200) {
      client.stop();
      timerOTA.reset();  // Reset timer
      return;
    }

    String rBody = client.responseBody();
    JsonDocument root;
    DeserializationError error = deserializeJson(root, rBody);
    if (error) {
      //Serial.println("Failed to read file, using default configuration");
      client.stop();
      timerOTA.reset();  // Reset timer
      return;
    }

    String arch;
#if defined(ARDUINO_ESP8266_WEMOS_D1R1) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_ESP8266_WEMOS_D1MINILITE)
    if (root["WEMOS"].isNull()) {
      client.stop();
      timerOTA.reset();  // Reset timer
      return;
    }
    arch = "WEMOS";
    //const char* PATH = "/devs/ota/ac/wemos-v%d.bin";
#else
    if (root["ESP8266"].isNull()) {
      client.stop();
      timerOTA.reset();  // Reset timer
      return;
    }
    arch = "ESP8266";
    //const char* PATH = "/devs/ota/ac/nodemcu-v%d.bin";
#endif

    if (!(VERSION < root[arch]["version"])) {
      client.stop();
      timerOTA.reset();  // Reset timer
      return;
    }

    /* TODO DATA[]
    if (!root[arch]["DATA"].isNull()) {
      
      for (String filename : root[arch]["DATA"]) {

      }
      
    }
    */

    char buff[64];
    strlcpy(buff, root[arch]["path"], sizeof(buff));

    //Serial.print("Check for update file ");
    //Serial.println(buff);

    client.get(buff);

    statusCode = client.responseStatusCode();
    //Serial.print("Update status code: ");
    //Serial.println(statusCode);
    if (statusCode != 200) {
      if (forceupdate) {
        server.send(statusCode, "text/html", "Manual update");
        delay(100);
      }
      client.stop();
      timerOTA.reset();  // Reset timer
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
      timerOTA.reset();  // Reset timer
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
      timerOTA.reset();  // Reset timer
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
      timerOTA.reset();  // Reset timer
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
}

bool checkforupdate() {

  const char* SERVER = deviceSettings.otaURL;
  const unsigned short SERVER_PORT = deviceSettings.otaURLPort;
  const char* PATH = deviceSettings.otaPath;

  WiFiClient wifiClient;
  HttpClient client(wifiClient, SERVER, SERVER_PORT);

  client.get(PATH);

  int statusCode = client.responseStatusCode();
  //Serial.print("Update status code: ");
  //Serial.println(statusCode);
  if (statusCode != 200) {
    client.stop();
    return false;
  }

  String rBody = client.responseBody();
  JsonDocument root;
  DeserializationError error = deserializeJson(root, rBody);
  if (error) {
    //Serial.println("Failed to read file, using default configuration");
    client.stop();
    return false;
  }

  String arch;
#if defined(ARDUINO_ESP8266_WEMOS_D1R1) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_WEMOS_D1MINIPRO) || defined(ARDUINO_ESP8266_WEMOS_D1MINILITE)
  if (root["WEMOS"].isNull()) {
    client.stop();
    return false;
  }
  arch = "WEMOS";
#else
  if (root["ESP8266"].isNull()) {
    client.stop();
    return false;
  }
  arch = "ESP8266";
#endif

  if (!(VERSION < root[arch]["version"])) {
    client.stop();
    return false;
  }

  client.stop();
  return true;
}

/*################################ OTA  ################################*/

/*################################ WEB  ################################*/

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

// Serving servCheckforupdate
void servCheckforupdate() {
  bool result = checkforupdate();
  if (result) {
    server.send(200, "application/json", "{\"result\":true}");
  } else {
    server.send(400, "application/json", "{\"result\":false}");
  }
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
    sendMQTT("{\"rst\":true,\"ep\":\"state\", \"message\":" + GetStateString() + "}");
  }

  server.send(200, "text/html", "reset");
  delay(300);
  ESP.restart();
}

// Format filesystem
void servFFS() {
  JsonDocument root;
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (!root["key"].isNull()) {
      if ((root["key"] == 56)) {
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
  server.send(200, "application/json", GetStateString());
}

// Serving getDeviceSettings
void getDeviceSettings() {
  server.send(200, "application/json", GetSettingsString());
}

// Serving getSyncMe
void getSyncMe() {
  server.send(200, "text/plain", ((deviceSettings.syncMe) ? "true" : "false"));
}

// Serving postState
void postState() {

  JsonDocument root;
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (!root["temp"].isNull()) {
      acState.temperature = (uint8_t)root["temp"];
    }
    if (!root["fan"].isNull()) {
      acState.fan = (uint8_t)root["fan"];
    }
    if (!root["power"].isNull()) {
      acState.powerStatus = root["power"];
    }
    if (!root["mode"].isNull()) {
      acState.operation = root["mode"];
    }
    if (!root["swingvert"].isNull()) {
      acState.swingvert = root["swingvert"];
    }
    if (!root["swinghor"].isNull()) {
      acState.swinghor = root["swinghor"];
    }
    if (!root["econo"].isNull()) {
      acState.econo = root["econo"];
    }
    if (!root["lowNoise"].isNull()) {
      acState.lowNoise = root["lowNoise"];
    }
    if (!root["heatTenC"].isNull()) {
      acState.heatTenC = root["heatTenC"];
    }

    String output = GetStateString();
    server.send(200, "application/json", output);

    delay(200);

    if (acState.powerStatus) {

      /*
      Serial.println("Power On / Change params");
      Serial.print("operation: ");Serial.println(acState.operation);
      Serial.print("fan: ");Serial.println(acState.fan);
      Serial.print("temperature: ");Serial.println(acState.temperature);
      Serial.print("swingvert: ");Serial.println(acState.swingvert);
      Serial.print("swinghor: ");Serial.println(acState.swinghor);
      */

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

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"state\", \"message\":" + output + "}");

    syncOtherDevices(output, "/state");
  }
}

// Serving postTurbo
void postTurbo() {

  JsonDocument root;
  DeserializationError error = deserializeJson(root, server.arg("plain"));
  if (error) {
    server.send(404, "text/plain", "FAIL. " + server.arg("plain"));
  } else {

    if (root["turbo"]) {
      acState.econo = false;
    }

    String output;
    serializeJson(root, output);

    server.send(200, "application/json", output);

    delay(200);

    if (root["turbo"]) {

      //Serial.println("Powerful mode On");

      irsend.setCmd(kFujitsuAcCmdPowerful);

      SendIR();

      sendMQTT("{\"islocal\":true,\"ep\":\"turbo\", \"message\":" + GetStateString() + "}");

      syncOtherDevices(output, "/turbo");
    }
  }
}

// Serving postStepV
void postStepV() {

  String message = server.arg("plain");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    server.send(404, "text/plain", "FAIL. " + message);
    return;
  }

  if (!root["swingvert"].isNull()) {
    acState.swingvert = root["swingvert"];
  }
  /*
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
  */
  //String output;
  //serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", message);

  delay(200);

  if (!root["stepv"].isNull() && root["stepv"]) {

    //Serial.println("Step vertical");

    irsend.setCmd(kFujitsuAcCmdStepVert);

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"stepv\", \"message\":" + GetStateString() + "}");

    syncOtherDevices(message, "/stepv");
  }
}

// Serving postStepH
void postStepH() {

  String message = server.arg("plain");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    server.send(404, "text/plain", "FAIL. " + message);
    return;
  }

  if (!root["swinghor"].isNull()) {
    acState.swinghor = root["swinghor"];
  }
  /*
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
  */
  //String output;
  //serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", message);

  delay(200);

  if (!root["steph"].isNull() && root["steph"]) {

    //Serial.println("Step horizontal");

    irsend.setCmd(kFujitsuAcCmdStepHoriz);

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"steph\", \"message\":" + GetStateString() + "}");

    syncOtherDevices(message, "/steph");
  }
}

// Serving postEcono
void postEcono() {

  String message = server.arg("plain");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    server.send(404, "text/plain", "FAIL. " + message);
    return;
  }

  if (!root["econo"].isNull()) {
    acState.econo = root["econo"];
  }
  /*
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
  */
  //String output;
  //serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", message);

  delay(200);

  if (!root["econo"].isNull()) {

    //Serial.print("Economy: ");Serial.println(acState.econo);
    irsend.setCmd(kFujitsuAcCmdEcono);

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"econo\", \"message\":" + GetStateString() + "}");

    syncOtherDevices(message, "/econo");
  }
}

// Serving postLNoise
void postLNoise() {

  String message = server.arg("plain");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    server.send(404, "text/plain", "FAIL. " + message);
    return;
  }

  if (!root["lowNoise"].isNull()) {
    acState.lowNoise = root["lowNoise"];
  }
  /*
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
  */
  //String output;
  //serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", message);

  delay(200);

  if (!root["lowNoise"].isNull()) {

    //Serial.println("Outdoor Unit Low Noise");
    //irsend.setCmd(kFujitsuAcOutsideQuietOffset);
    if (acState.lowNoise) {
      irsend.setOutsideQuiet(true);
    } else {
      irsend.setOutsideQuiet(false);
    }

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"lnoise\", \"message\":" + GetStateString() + "}");

    syncOtherDevices(message, "/lnoise");
  }
}

// Serving postHeatTenC
void postHeatTenC() {

  String message = server.arg("plain");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, message);
  if (error) {
    server.send(404, "text/plain", "FAIL. " + message);
  }

  if (!root["heatTenC"].isNull()) {
    acState.heatTenC = root["heatTenC"];
  }
  /*
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
 */
  //String output;
  //serializeJson(root, output);
  //server.send(200, "text/plain", output);
  server.send(200, "application/json", message);

  delay(200);

  if (!root["heatTenC"].isNull()) {
    //Serial.println("Set 10C Heat");

    if (acState.heatTenC) {
      irsend.set10CHeat(true);
    } else {
      irsend.set10CHeat(false);
    }

    SendIR();

    sendMQTT("{\"islocal\":true,\"ep\":\"heattenc\", \"message\":" + GetStateString() + "}");

    syncOtherDevices(message, "/heattenc");
  }
}

// Serving postDeviceSettings
void postDeviceSettings() {

  String newMessage = server.arg("plain");
  String output = SetDeviceSettings(newMessage);

  if (output == "404") {
    server.send(404, "text/plain", "FAIL. " + newMessage);
  } else {
    server.send(200, "application/json", output);
  }
}

void syncOtherDevices(String output, String endPoint) {

  //Serial.println("Sync: " + endPoint);
  //Serial.println(output);

  if (deviceSettings.synchronize) {

    File file = FILESYSTEM.open(filenameS, "r");
    //  }

    JsonDocument root;
    DeserializationError error = deserializeJson(root, file);
    if (error || root["syncDevs"].isNull()) {
      //Serial.println("Failed to read file, using default configuration");
      return;
    }

    int statusCode;
    String response;
    WiFiClient wifiClient;

    int _count = root["syncDevs"].size();

    for (int i = 0; i <= _count; i++) {

      if (!root["syncDevs"][i]["enable"]) {
        continue;
      }

      const char* SERVER = root["syncDevs"][i]["devURL"];                 // must be string for HttpClient
      const unsigned short SERVER_PORT = root["syncDevs"][i]["devPort"];  //80;

      HttpClient client(wifiClient, SERVER, SERVER_PORT);
      client.get("/syncMe");

      // read the status code and body of the response
      statusCode = client.responseStatusCode();
      response = client.responseBody();
      if ((statusCode == 200) && (response == "true")) {
        client.post(endPoint, "application/json", output);
      }
      //Serial.print("Status code GET: "); Serial.println(statusCode);
      //Serial.print("Response GET: "); Serial.println(response);
    }
    //Serial.println("Wait five seconds");
    //delay(5000);
  }
}

/*################################ WEB  ################################*/

/*############################### Setup  ###############################*/

void setUpTimers() {

  timerMQTT.setInterval(30000);
  timerWiFi.setInterval(30000);
  timerOTA.setInterval(60000);
}

void reconnectMQTT() {

  if (!deviceSettings.useMQTT) {
    return;
  }

  mqttClient.poll();
  if (timerMQTT.isReady()) {  // Check is ready a second timer

    if (!mqttClient.connected()) {

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
    }

    timerMQTT.reset();  // Reset timer
  }
}

void reconnectWiFi() {

  if (timerWiFi.isReady()) {
    //unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED)) {  // && (currentMillis - wifiPreviousMillis >= wifiReconnectInterval)) {
      //Serial.print(millis());Serial.print(" - ");Serial.println("Reconnecting to WiFi...");
      //WiFi.disconnect(false);
      WiFi.reconnect();
      //wifiPreviousMillis = currentMillis;
    }
    timerWiFi.reset();  // Reset timer
  }
}

void setWebServer() {

  httpUpdateServer.setup(&server);

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
  server.on("/checkforupdate", HTTP_GET, servCheckforupdate);
  server.on("/forceupdate", HTTP_POST, servForceupdate);
  server.on("/upload", HTTP_GET, []() {                  // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                 // send it if it exists
      server.send(404, "text/plain", "404: Not Found");  // otherwise, respond with a 404 (Not Found) error
  });
  server.on(
    "/upload", HTTP_POST,  // if the client posts to the upload page
    []() {},
    handleFileUpload);  // Receive and save the file
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

/*############################### Setup  ###############################*/

void setup() {

  /*
  Serial.begin(kBaudRate, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("Sketch version ");
  Serial.println(VERSION);
  */
  /*
  Serial.print("IR input Pin ");
  Serial.println(kRecvPin);
  Serial.print("IR transmit Pin ");
  Serial.println(kIrLedPin);
  */

  //Serial.println("mounting " FILESYSTEMSTR "...");

  if (!FILESYSTEM.begin()) {
    //Serial.println("Failed to mount file system");
    return;
  }

  ReadDeviceSettings();

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

  // TIMERS
  setUpTimers();
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
