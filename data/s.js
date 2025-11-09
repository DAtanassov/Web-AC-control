var settings = {};

function getSettings() {

  fetch("settings")
    .then(r => r.json())
    .then(data => {
      settings = data;
      updateElements();
    })
    .catch(error => {
      console.error(error);
      updateElements();
    });

}

function updateElements() {

  const el = document.getElementById("updateMessage");
  el.innerHTML = "";
  el.classList.remove("text-danger", "text-success");

  // update input elements by name
  for (const [name, val] of Object.entries(settings)) {

    const elements = document.querySelectorAll(`[name="${name}"]`);
    elements.forEach(e => {
      const type = e.type;
      switch (type) {
        case "checkbox":
          e.checked = !!val;
          break;
        case "radio":
          if (e.value == val) e.checked = true;
          break;
        default:
          e.value = Array.isArray(val) ? val.join(",") : val;
      }
    });
  }

  document.title = `${settings.deviceName} (AC WiFi IR Control Settings v.${settings.version})`;
  showSyncDevs();

  document.getElementById("mqttGroup").style.display = document.getElementById("useMQTT").checked ? "block" : "none";
  document.getElementById("syncDevsGroup").style.display = document.getElementById("synchronize").checked ? "block" : "none";
}

function postData(t, p) {

  fetch(p, {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json;charset=UTF-8"
    },
    body: JSON.stringify(t)
  }).catch(error => {
    console.error(error);
  });

}

function setSettings() {

  const v = id => document.getElementById(id).value;
  const c = id => document.getElementById(id).checked;

  settings.deviceName = v("deviceName");
  settings.wifiPass = v("wifiPass");
  settings.wifiChannel = parseInt(v("wifiChannel"), 10);
  settings.startAP = c("startAP");
  settings.hideSSID = c("hideSSID");
  // Sync
  settings.synchronize = c("synchronize");
  settings.syncMe = c("syncMe");
  settings.enableIRRecv = c("enableIRRecv");

  const table = document.getElementById("devicesTBody");
  const rows = table.querySelectorAll("tr");
  const syncDevs = [];
  rows.forEach((row, i) => {
    const cells = row.cells;
    syncDevs.push({
      enable: cells[1].querySelector("input").checked,
      devURL: cells[2].querySelector("input").value,
      devPort: parseInt(cells[3].querySelector("input").value),
      devName: cells[4].querySelector("input").value
    });
  });
  settings.syncDevs = syncDevs;
  // OTA
  settings.otaAutoUpd = c("otaAutoUpd");
  settings.otaURL = v("otaURL");
  settings.otaURLPort = parseInt(v("otaURLPort"), 10);
  settings.otaPath = v("otaPath");
  // MQTT
  settings.useMQTT = c("useMQTT");
  settings.mqtt_broker = v("mqtt_broker");
  settings.mqtt_port = parseInt(v("mqtt_port"), 10);
  settings.mqtt_topic = v("mqtt_topic");
  settings.mqtt_username = v("mqtt_username");
  settings.mqtt_password = v("mqtt_password");
  // Model
  settings.irModel = parseInt(v("irModel"), 10);

  postData(settings, "settings");

}

function checkForUpdate() {

  const el = document.getElementById("updateMessage");

  fetch("checkforupdate")
    .then(r => r.json())
    .then(rM => {
      document.getElementById("buttonFU").disabled = rM.result === false;
      el.classList.remove("text-danger");
      el.classList.add("text-success");
      el.innerHTML = `<h6>${rM.result ? "New version released!" : "No new version found."}</h6>`;
    })
    .catch(error => {
      document.getElementById("buttonFU").disabled = true;
      el.classList.remove("text-success");
      el.classList.add("text-danger");
      el.innerHTML = `<h6>${error}</h6>`;
      console.error(error);
    });
}

function updateScetch() {
  
  fetch("forceupdate", {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json;charset=UTF-8"
    },
    body: ""
  })
    .then(r => r.text())
    .then(response => {
      document.getElementById("buttonFU").disabled = true;
      document.getElementById("updateMessage").innerHTML = `<h6>${response}</h6>`;
    })
    .catch(error => {
      document.getElementById("buttonFU").disabled = true;
      document.getElementById("updateMessage").innerHTML = `<h6>${error}</h6>`;
      console.error(error);
    });
}

function devRestart() {
  postData(null, "reset");
}

function showSyncDevs() {
  const tbody = document.getElementById("devicesTBody");
  tbody.innerHTML = "";
  const devs = settings.syncDevs || [];
  devs.forEach(d =>
    addRowDevsButtons(d.enable ? "checked" : "", d.devURL, d.devPort, d.devName)
  );
}

function addRowDevsButtons(dEnable = "", dURL = "", dPort = 80, dName = "") {
  
  const table = document.getElementById("devicesTBody");
  const row = table.insertRow();
  const cells = [];
  
  for (let i = 0; i < 6; i++) cells[i] = row.insertCell(i);

  cells[0].innerHTML = `<a id="rowID">${row.rowIndex}</a>`;
  cells[1].innerHTML = `<input class="form-check-input" type="checkbox" role="switch" ${dEnable}>`;
  cells[2].innerHTML = `<input type="text" class="form-control" value="${dURL}">`;
  cells[3].innerHTML = `<input type="number" min="1" max="65535" class="form-control" value="${dPort}">`;
  cells[4].innerHTML = `<input type="text" class="form-control" value="${dName}">`;
  cells[5].colSpan = 2;
  cells[5].innerHTML = `<button type="button" class="form-control btn btn-outline-danger" onclick="deleteRowDevsButtons(this)">
    <i class="far fa-trash-alt"></i>&nbsp; Del</button>`;
}

function deleteRowDevsButtons(btn) {
  const table = document.getElementById("devicesTBody");
  const rowIndex = btn.closest("tr").rowIndex;
  table.deleteRow(rowIndex - 1);
  Array.from(table.rows).forEach((row, i) => {
    row.cells[0].innerHTML = `<a id="rowID">${i + 1}</a>`;
  });
}

document.addEventListener("DOMContentLoaded", () => {

  const useMQTT = document.getElementById("useMQTT");
  const synchronize = document.getElementById("synchronize");

  if (useMQTT)
    useMQTT.addEventListener("click", () => {
      document.getElementById("mqttGroup").style.display = useMQTT.checked ? "block" : "none";
    });

  if (synchronize)
    synchronize.addEventListener("click", () => {
      document.getElementById("syncDevsGroup").style.display = synchronize.checked ? "block" : "none";
    });
});
