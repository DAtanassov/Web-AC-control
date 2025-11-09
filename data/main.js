var state = {};
var setTemp;

function updateStatus() {
  fetch('state')
    .then(response => response.json())
    .then(data => {
      state = data;
      updateElements();
    })
    .catch(error => {
      console.error(error);
      updateElements();
    });
}

function updateElements() {

  const powerText = document.getElementById("power");
  const powerBtn = document.getElementById("power-btn");
  const swingBtn = document.getElementById("swing-btn");
  const targetTemp = document.getElementById("target_temp");
  const econoBtn = document.getElementById("econo-btn");
  const lowNoiseBtn = document.getElementById("lowNoise-btn");
  const heat10Btn = document.getElementById("heat10-btn");
  const swingHBtn = document.getElementById("swingH-btn");
  const devName = document.getElementById("devName");

  // Power
  if (state.power) {
    powerText.textContent = " ON";
    powerBtn.classList.add("btn-success");
    powerBtn.classList.remove("btn-danger");
  } else {
    powerText.textContent = " OFF";
    powerBtn.classList.remove("btn-success");
    powerBtn.classList.add("btn-danger");
  }

  // Swing vertical
  if (state.swingvert) {
    swingBtn.classList.remove("btn-outline-info");
    swingBtn.classList.add("btn-info");
  } else {
    swingBtn.classList.remove("btn-info");
    swingBtn.classList.add("btn-outline-info");
  }

  // Temperature
  targetTemp.textContent = state.temp + " °C";

  // Econo
  if (state.econo) {
    econoBtn.classList.remove("btn-outline-info");
    econoBtn.classList.add("btn-info");
  } else {
    econoBtn.classList.remove("btn-info");
    econoBtn.classList.add("btn-outline-info");
  }

  // Low Noise
  if (state.lowNoise) {
    lowNoiseBtn.classList.remove("btn-outline-info");
    lowNoiseBtn.classList.add("btn-info");
  } else {
    lowNoiseBtn.classList.remove("btn-info");
    lowNoiseBtn.classList.add("btn-outline-info");
  }

  // Heat 10°C
  if (state.heatTenC) {
    heat10Btn.classList.remove("btn-outline-info");
    heat10Btn.classList.add("btn-info");
    targetTemp.textContent = "10 °C";
  } else {
    heat10Btn.classList.remove("btn-info");
    heat10Btn.classList.add("btn-outline-info");
  }

  // Hide horizontal swing if not supported
  if (!(state.irModel === 1 || state.irModel === 4)) {
    const sh = document.getElementById("swingH-btn");
    const stepH = document.getElementById("stepHor-btn");
    if (sh) sh.remove();
    if (stepH) stepH.remove();
  }

  // Swing horizontal
  if (state.swinghor) {
    swingHBtn.classList.remove("btn-outline-info");
    swingHBtn.classList.add("btn-info");
  } else {
    swingHBtn.classList.remove("btn-info");
    swingHBtn.classList.add("btn-outline-info");
  }

  setModeColor(state.mode);
  setFanColor(state.fan);

  document.title = `${state.deviceName} v.${state.version}`;
  devName.textContent = state.deviceName;
}

function postData(t, p) {
  fetch(p, {
    method: "POST",
    headers: {
      Accept: "application/json",
      "Content-Type": "application/json;charset=UTF-8"
    },
    body: JSON.stringify(t)
  })
    .catch(error => {
      console.error(error);
    });
}

// ------------------------- MODE -------------------------

function mode_onclick(mode) {
  state.mode = mode;
  setModeColor(mode);
  if (state.power) postData(state, "state");
}

function setModeColor(mode) {
  
  document.querySelectorAll(".mode-btn").forEach(btn => {
    btn.classList.remove("btn-info");
    btn.classList.add("btn-outline-info");
  });

  const map = {
    0: "mode_auto",
    1: "mode_cooling",
    2: "mode_dehum",
    3: "mode_fan",
    4: "mode_heating"
  };
  const active = document.getElementById(map[mode]);
  if (active) {
    active.classList.remove("btn-outline-info");
    active.classList.add("btn-info");
  }
}

// ------------------------- FAN -------------------------

function setFanColor(fan) {
  const fanAuto = document.getElementById("fan_auto");
  if (fan === 0) {
    fanAuto.classList.remove("btn-outline-info");
    fanAuto.classList.add("btn-info");
  } else {
    fanAuto.classList.remove("btn-info");
    fanAuto.classList.add("btn-outline-info");
  }

  for (let i = 1; i <= 4; i++) {
    const icon = document.getElementById("fan_lvl_" + i);
    if (icon)
      icon.setAttribute(
        "xlink:href",
        `imgsvg.svg#level_${i}_${i <= fan ? "on" : "off"}`
      );
  }
}

function fan_onclick(fan) {
  state.fan = fan;
  setFanColor(fan);
  if (state.power) postData(state, "state");
}

// ------------------------- POWER -------------------------

function power_onclick() {
  const powerText = document.getElementById("power");
  const powerBtn = document.getElementById("power-btn");

  state.power = !state.power;

  if (state.power) {
    powerText.textContent = " ON";
    powerBtn.classList.add("btn-success");
    powerBtn.classList.remove("btn-danger");
  } else {
    powerText.textContent = " OFF";
    powerBtn.classList.remove("btn-success");
    powerBtn.classList.add("btn-danger");
  }

  postData(state, "state");
}

// ------------------------- TEMP -------------------------

function temp_onclick(temp) {
  state.temp += temp;
  if (state.mode == 1 && state.temp < 18) state.temp = 18;
  else if (state.temp < 16) state.temp = 16;
  if (state.temp > 30) state.temp = 30;

  document.getElementById("target_temp").textContent = state.temp + " °C";

  if (state.power) {
    clearTimeout(setTemp);
    setTemp = setTimeout(() => postData(state, "state"), 1000);
  }
}

// ------------------------- SWING VERTICAL -------------------------

function swing_onclick() {
  const btn = document.getElementById("swing-btn");
  state.swingvert = !state.swingvert;
  btn.classList.toggle("btn-info", state.swingvert);
  btn.classList.toggle("btn-outline-info", !state.swingvert);
  if (state.power) postData(state, "state");
}

function StepV_onclick() {
  const btn = document.getElementById("swing-btn");
  state.swingvert = false;
  btn.classList.remove("btn-info");
  btn.classList.add("btn-outline-info");
  if (state.power) postData({ stepv: true, swingvert: false }, "stepv");
}

// ------------------------- ECONO -------------------------

function econo_onclick() {
  const btn = document.getElementById("econo-btn");
  state.econo = !state.econo;
  btn.classList.toggle("btn-info", state.econo);
  btn.classList.toggle("btn-outline-info", !state.econo);
  if (state.power) postData(state, "econo");
}

// ------------------------- TURBO -------------------------

function turbo_onclick() {
  if (state.power) postData({ turbo: true }, "turbo");
}

// ------------------------- LOW NOISE -------------------------

function lowNoise_onclick() {
  const btn = document.getElementById("lowNoise-btn");
  state.lowNoise = !state.lowNoise;
  btn.classList.toggle("btn-info", state.lowNoise);
  btn.classList.toggle("btn-outline-info", !state.lowNoise);
  if (state.power) postData(state, "lnoise");
}

// ------------------------- HEAT 10°C -------------------------

function heat10_onclick() {
  const btn = document.getElementById("heat10-btn");
  const tempLabel = document.getElementById("target_temp");

  state.heatTenC = !state.heatTenC;
  btn.classList.toggle("btn-info", state.heatTenC);
  btn.classList.toggle("btn-outline-info", !state.heatTenC);
  tempLabel.textContent = state.heatTenC ? "10 °C" : state.temp + " °C";

  if (state.power) postData(state, "heattenc");
}

// ------------------------- SWING HORIZONTAL -------------------------

function swingH_onclick() {
  const btn = document.getElementById("swingH-btn");
  state.swinghor = !state.swinghor;
  btn.classList.toggle("btn-info", state.swinghor);
  btn.classList.toggle("btn-outline-info", !state.swinghor);
  if (state.power) postData(state, "state");
}

function StepH_onclick() {
  const btn = document.getElementById("swingH-btn");
  state.swinghor = false;
  btn.classList.remove("btn-info");
  btn.classList.add("btn-outline-info");
  if (state.power) postData({ steph: true, swinghor: false }, "steph");
}
