var state = {};
var settings = {};
var setTemp;

function updateStatus() {
  fetch('state')
  .then(async (response) => {
    state = await response.json();
    updateElements();
  })
  .catch(error => {
    console.error(error);
  });
}

function updateAll() {
  Promise.all([
    fetch('settings'),
    fetch('state')
  ])
    .then(async ([res1, res2]) => {
      settings = await res1.json();
      state = await res2.json();
      updateElements();
    })
    .catch(error => {
      console.error(error);
    });
}

function updateElements() {
  if (state["power"] === true) {
    $("#power").text(" ON");
    $("#power-btn").addClass("btn-success");
    $("#power-btn").removeClass("btn-danger");
  } else {
    $("#power").text(" OFF");
    $("#power-btn").removeClass("btn-success");
    $("#power-btn").addClass("btn-danger");
  }
  if (state["swingvert"] === true) {
    $("#swing-btn").removeClass("btn-outline-info");
    $("#swing-btn").addClass("btn-info");
  } else {
    $("#swing-btn").removeClass("btn-info");
    $("#swing-btn").addClass("btn-outline-info");
  }
  $("#target_temp").text(state["temp"] + " °C");
  if (state["econo"] === true) {
    $("#econo-btn").removeClass("btn-outline-info");
    $("#econo-btn").addClass("btn-info");
  } else {
    $("#econo-btn").removeClass("btn-info");
    $("#econo-btn").addClass("btn-outline-info");
  }
  if (state["lowNoise"] === true) {
    $("#lowNoise-btn").removeClass("btn-outline-info");
    $("#lowNoise-btn").addClass("btn-info");
  } else {
    $("#lowNoise-btn").removeClass("btn-info");
    $("#lowNoise-btn").addClass("btn-outline-info");
  }
  if (state["heatTenC"] === true) {
    $("#heat10-btn").removeClass("btn-outline-info");
    $("#heat10-btn").addClass("btn-info");
    $("#target_temp").text("10 °C");
  } else {
    $("#heat10-btn").removeClass("btn-info");
    $("#heat10-btn").addClass("btn-outline-info");
  }
  if (!((settings["irModel"] === 1) || (settings["irModel"] === 4))) {
    $("#swingH-btn").remove();
    $("#stepHor-btn").remove();
  }
  if (state["swinghor"] === true) {
    $("#swingH-btn").removeClass("btn-outline-info");
    $("#swingH-btn").addClass("btn-info");
  } else {
    $("#swingH-btn").removeClass("btn-info");
    $("#swingH-btn").addClass("btn-outline-info");
  }
  setModeColor(state["mode"]);
  setFanColor(state["fan"]);
  document.title = state["deviceName"] + " v." + state["version"];
  $("#devName").text(state["deviceName"]);
}

function postData(t, p) {
  //return;
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

function mode_onclick(mode) {
  state["mode"] = mode;
  setModeColor(mode);
  if (state["power"] === true) {
    postData(state, "state");
  }
}

function setModeColor(mode) {
  $(".mode-btn").removeClass("btn-info");
  $(".mode-btn").addClass("btn-outline-info");
  if (mode === 0) {
    $("#mode_auto").removeClass("btn-outline-info");
    $("#mode_auto").addClass("btn-info");

  } else if (mode === 1) {
    $("#mode_cooling").removeClass("btn-outline-info");
    $("#mode_cooling").addClass("btn-info");
  } else if (mode === 2) {
    $("#mode_dehum").removeClass("btn-outline-info");
    $("#mode_dehum").addClass("btn-info");
  } else if (mode === 3) {
    $("#mode_fan").removeClass("btn-outline-info");
    $("#mode_fan").addClass("btn-info");
  } else if (mode === 4) {
    $("#mode_heating").removeClass("btn-outline-info");
    $("#mode_heating").addClass("btn-info");
  }
}

function setFanColor(fan) {
  if (fan == 0) {
    $("#fan_auto").removeClass("btn-outline-info");
    $("#fan_auto").addClass("btn-info");
  } else {
    $("#fan_auto").removeClass("btn-info");
    $("#fan_auto").addClass("btn-outline-info");
  }
  for (var i = 1; i <= 4; ++i) {
    if (i <= fan) {
      $("#fan_lvl_" + i).attr("xlink:href", "imgsvg.svg#level_" + i + "_on");
    } else {
      $("#fan_lvl_" + i).attr("xlink:href", "imgsvg.svg#level_" + i + "_off");
    }
  }
}

function fan_onclick(fan) {
  state["fan"] = fan;
  setFanColor(fan);
  if (state["power"] === true) {
    postData(state, "state");
  }
}

function power_onclick() {
  if (state["power"] === true) {
    state["power"] = false;
    $("#power").text(" OFF");
    $("#power-btn").removeClass("btn-success");
    $("#power-btn").addClass("btn-danger");
  } else {
    state["power"] = true;
    $("#power").text(" ON");
    $("#power-btn").removeClass("btn-danger");
    $("#power-btn").addClass("btn-success");
  }
  postData(state, "state");
}

function temp_onclick(temp) {
  state["temp"] += temp;
  if (state["mode"] == 1) {
    if (state["temp"] < 18) {
      state["temp"] = 18;
    }
  } else {
    if (state["temp"] < 16) {
      state["temp"] = 16;
    }
  }
  if (state["temp"] > 30) {
    state["temp"] = 30;
  }
  $("#target_temp").text(state["temp"] + " °C");
  if (state["power"] === true) {
    clearTimeout(setTemp);
    setTemp = setTimeout(function () { postData(state, "state"); }, 1000);
  }
}

function swing_onclick() {
  if (state["swingvert"]) {
    state["swingvert"] = false;
    $("#swing-btn").removeClass("btn-info");
    $("#swing-btn").addClass("btn-outline-info");
  } else {
    state["swingvert"] = true;
    $("#swing-btn").removeClass("btn-outline-info");
    $("#swing-btn").addClass("btn-info");
  }
  if (state["power"] === true) {
    postData(state, "state");
  }
}

function StepV_onclick() {
  state["swingvert"] = false;
  $("#swing-btn").removeClass("btn-info");
  $("#swing-btn").addClass("btn-outline-info");
  if (state["power"] === true) {
    stepVState = { "stepv": true, "swingvert": false };
    postData(stepVState, "stepv");
  }
}

function econo_onclick() {
  if (state["econo"]) {
    state["econo"] = false;
    $("#econo-btn").removeClass("btn-info");
    $("#econo-btn").addClass("btn-outline-info");
  } else {
    state["econo"] = true;
    $("#econo-btn").removeClass("btn-outline-info");
    $("#econo-btn").addClass("btn-info");
  }
  if (state["power"] === true) {
    postData(state, "econo");
  }
}

function turbo_onclick() {
  if (state["power"] === true) {
    turboState = { "turbo": true };
    postData(turboState, "turbo");
  }
}

function lowNoise_onclick() {
  if (state["lowNoise"]) {
    state["lowNoise"] = false;
    $("#lowNoise-btn").removeClass("btn-info");
    $("#lowNoise-btn").addClass("btn-outline-info");
  } else {
    state["lowNoise"] = true;
    $("#lowNoise-btn").removeClass("btn-outline-info");
    $("#lowNoise-btn").addClass("btn-info");
  }
  if (state["power"] === true) {
    postData(state, "lnoise");
  }
}

function heat10_onclick() {
  if (state["heatTenC"]) {
    state["heatTenC"] = false;
    $("#heat10-btn").removeClass("btn-info");
    $("#heat10-btn").addClass("btn-outline-info");
    $("#target_temp").text(state["temp"] + " °C");
  } else {
    state["heatTenC"] = true;
    $("#heat10-btn").removeClass("btn-outline-info");
    $("#heat10-btn").addClass("btn-info");
    $("#target_temp").text("10 °C");
  }
  if (state["power"] === true) {
    postData(state, "heattenc");
  }
}

function swingH_onclick() {
  if (state["swinghor"]) {
    state["swinghor"] = false;
    $("#swingH-btn").removeClass("btn-info");
    $("#swingH-btn").addClass("btn-outline-info");
  } else {
    state["swinghor"] = true;
    $("#swingH-btn").removeClass("btn-outline-info");
    $("#swingH-btn").addClass("btn-info");
  }
  if (state["power"] === true) {
    postData(state, "state");
  }
}

function StepH_onclick() {
  state["swinghor"] = false;
  $("#swingH-btn").removeClass("btn-info");
  $("#swingH-btn").addClass("btn-outline-info");
  if (state["power"] === true) {
    stepVState = { "steph": true, "swinghor": false };
    postData(stepVState, "steph");
  }
}
