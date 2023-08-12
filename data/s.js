var settings = {};

function getSettings() {
  fetch('settings')
  .then(async(response) => {
    settings = await response.json();
    updateElements();
  })
  .catch(error => {
    console.error(error);
  });
}

function updateElements(){
  $.each(settings, function (name, val) {
    var $el = $('[name="' + name + '"]'),
        type = $el.attr('type');

    switch (type) {
        case 'checkbox':
            $el.prop('checked', val);
            break;
        case 'radio':
            $el.filter('[value="' + val + '"]').attr('checked', val);
            break;
        default:
            $el.val(!$.isArray(val) ? [val] : val);
    }
    })
  document.title = settings["deviceName"] + " (AC WiFi IR Control Settings v." + settings["version"] + ")";
  
  if($("#useMQTT").is(':checked')) {
    $("#mqttGroup").show();
  } else {
    $("#mqttGroup").hide();
  }
}

function postData(t, p) {
  fetch(p, {method: "POST",
            headers: {
              Accept: "application/json",
              "Content-Type": "application/json;charset=UTF-8",
            },
            body: JSON.stringify(t)})
  .catch(error => {
    console.error(error);
  });
}

function setSettings() {

    settings["deviceName"] = $("#deviceName").val();
    settings["wifiPass"] = $("#wifiPass").val();
    settings["wifiChannel"] = parseInt($("#wifiChannel").val(), 10);
    settings["startAP"] = $("#startAP").is(':checked') ? true : false;
    settings["hideSSID"] = $("#hideSSID").is(':checked') ? true : false;    
    settings["enableIRRecv"] = $("#enableIRRecv").is(':checked') ? true : false;
    // Sync
    settings["synchronise"] = $("#synchronise").is(':checked') ? true : false;
    settings["syncMe"] = $("#syncMe").is(':checked') ? true : false;
    settings["innerDoor"] = $("#innerDoor").is(':checked') ? true : false;
    settings["outerDoor"] = $("#outerDoor").is(':checked') ? true : false;
    // OTA
    settings["autoupdate"] = $("#autoupdate").is(':checked') ? true : false;
    settings["updSvr"] = $("#updSvr").val();
    settings["updSvrPort"] = parseInt($("#updSvrPort").val(), 10);
    // MQTT
    settings["useMQTT"] = $("#useMQTT").is(':checked') ? true : false;
    settings["mqtt_broker"] = $("#mqtt_broker").val();
    settings["mqtt_port"] = parseInt($("#mqtt_port").val(), 10);
    settings["mqtt_topic"] = $("#mqtt_topic").val();
    settings["mqtt_username"] = $("#mqtt_username").val();
    settings["mqtt_password"] = $("#mqtt_password").val();
    // Model
    settings["irModel"] = parseInt($("#irModel").val(), 10);

    postData(settings, "settings");
}

function checkUPD() {
  postData(null, "forceupdate");
}

function devRestart() {
  postData(null, "reset");
}

// MQTT
$(document).ready(function(){
  $("#useMQTT").click(function(){
    if($(this).is(':checked')) {
      $("#mqttGroup").show();
    } else {
      $("#mqttGroup").hide();
    }
  });
});
