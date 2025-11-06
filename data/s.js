var settings = {};

function getSettings() {
  fetch('settings')
    .then(async (response) => {
      settings = await response.json();
      updateElements();
    })
    .catch(error => {
      console.error(error);
      updateElements();
    });
}

function updateElements() {
  var el = document.getElementById("updateMessage");
  el.innerHTML = "";
  if (el.classList.contains("text-danger")) {
    el.classList.remove("text-danger")
  }else if (el.classList.contains("text-success")) {
    el.classList.remove("text-success")
  }
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
  showSyncDevs();

  if ($("#useMQTT").is(':checked')) {
    $("#mqttGroup").show();
  } else {
    $("#mqttGroup").hide();
  }
  if ($("#synchronize").is(':checked')) {
    $("#syncDevsGroup").show();
  } else {
    $("#syncDevsGroup").hide();
  }
}

function postData(t, p) {
  fetch(p, {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json;charset=UTF-8",
    },
    body: JSON.stringify(t)
  })
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
  // Sync
  settings["synchronize"] = $("#synchronize").is(':checked') ? true : false;
  settings["syncMe"] = $("#syncMe").is(':checked') ? true : false;
  settings["enableIRRecv"] = $("#enableIRRecv").is(':checked') ? true : false;

  table = document.getElementById("devicesTBody");
  var syncDevs = JSON.parse("[]");
  for (i = 0; i < table.childNodes.length; i++) {
    row = table.childNodes[i];
    syncDevs.push({});
    //cell0 = row.cells[0].childNodes[0];//id
    syncDevs[i]["enable"] = row.cells[1].childNodes[0].checked;//enable
    syncDevs[i]["devURL"] = row.cells[2].childNodes[0].value;//address
    syncDevs[i]["devPort"] = parseInt(row.cells[3].childNodes[0].value);//port
    syncDevs[i]["devName"] = row.cells[4].childNodes[0].value;//device name
  }
  settings["syncDevs"] = syncDevs;
  // OTA
  settings["otaAutoUpd"] = $("#otaAutoUpd").is(':checked') ? true : false;
  settings["otaURL"] = $("#otaURL").val();
  settings["otaURLPort"] = parseInt($("#otaURLPort").val(), 10);
  settings["otaPath"] = $("#otaPath").val();
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

function checkForUpdate() {
  fetch('checkforupdate')
    .then(async (response) => {
      rM = await response.json();
      $("#buttonFU").prop('disabled', rM.result === false);
      var el = document.getElementById("updateMessage");
      if (el.classList.contains("text-danger")) {
      el.classList.remove("text-danger")
      }
      el.classList.add("text-success");
      if (rM.result === true) {
        el.innerHTML = "<h6>New version releised!</h6>";
      }else {
        el.innerHTML = "<h6>No new version found.</h6>";
      }
    })
    .catch(error => {
      $("#buttonFU").prop('disabled', true);
      var el = document.getElementById("updateMessage");
      if (el.classList.contains("text-success")) {
        el.classList.remove("text-success")
      }
      el.classList.add("text-danger");
      el.innerHTML = "<h6>" + error + "</h6>";
      console.error(error);
    });
}

function updateScetch() {
  fetch('forceupdate', {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json;charset=UTF-8",
    },
    body: ""})
    .then(async (response) => response.text())
    .then((response) => {

      $("#buttonFU").prop('disabled', true);
      var el = document.getElementById("updateMessage");
      el.innerHTML = "<h6>" + response + "</h6>";

    })
    .catch(error => {
      $("#buttonFU").prop('disabled', true);
      var el = document.getElementById("updateMessage");
      el.innerHTML = "<h6>" + error + "</h6>";
      console.error(error);
    });

}


function devRestart() {
  postData(null, "reset");
}

// syncDevs
function showSyncDevs() {

  $("#devicesTBody tr").remove();
  devs = settings.syncDevs;
  for (i in devs) {
    addRowDevsButtons((devs[i].enable ? "checked" : ""), devs[i].devURL, devs[i].devPort, devs[i].devName);
  };
}

function addRowDevsButtons(dEnable = "", dURL = "", dPort = 80, dName = "") {

  var table = document.getElementById("devicesTBody");

  var row = table.insertRow();
  var cell0 = row.insertCell(0);
  var cell1 = row.insertCell(1);
  var cell2 = row.insertCell(2);
  var cell3 = row.insertCell(3);
  var cell4 = row.insertCell(4);
  var cell5 = row.insertCell(5);
  cell0.innerHTML = "<a id='rowID'>" + row.rowIndex + "</a>";
  cell1.innerHTML = "<input class='form-check-input' type='checkbox' role='switch' id='enable'" + dEnable + ">";
  cell2.innerHTML = "<input type='text' class='form-control' id='devURL' aria-describedby='basic-addon3' value='" + dURL + "'>";
  cell3.innerHTML = "<input type='number' maxlength='5' min='1' max='65535' class='form-control' id='devPort' aria-describedby='basic-addon3' value='" + dPort + "'>";
  cell4.innerHTML = "<input type='text' class='form-control' id='devName' aria-describedby='devName' value='" + dName + "'>";
  cell5.colSpan = 2;
  cell5.innerHTML = "<button type='button' class='form-control btn btn-outline-danger' onclick='deleteRowDevsButtons(this)'><i class='far fa-trash-alt'></i>&nbsp; Del</button>";

}

function deleteRowDevsButtons(r) {
  var i = r.parentNode.parentNode.rowIndex;
  table = document.getElementById("devicesTBody");
  table.deleteRow(i - 1);
  for (i = 0; i < table.childNodes.length; i++) {
    row = table.childNodes[i].cells[0].innerHTML = "<a id='rowID'>" + (i + 1) + "</a>";
  }
}

$(document).ready(function () {
  $("#useMQTT").click(function () {
    if ($(this).is(':checked')) {
      $("#mqttGroup").show();
    } else {
      $("#mqttGroup").hide();
    }
  });
  $("#synchronize").click(function () {
    if ($(this).is(':checked')) {
      $("#syncDevsGroup").show();
    } else {
      $("#syncDevsGroup").hide();
    }
  });
});
