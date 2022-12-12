var state = {};
var settings = {};
var setTemp;

function updateStatus() {
    
  fetch('state')
  .then(async(response) => {
    state = await response.json();
    updateElements();
  })
  .catch(error => {
    console.error(error);
  });
    
  /* var xhr = new XMLHttpRequest();     
  xhr.open("GET", "state", false);
  xhr.send(null);
  if (xhr.status === 200) {
    state = JSON.parse(xhr.responseText);
    updateElements();
  } */
  
}

function updateAll() {
   
  Promise.all([
    fetch('settings'),
    fetch('state')
  ])
  .then(async([res1, res2]) => {
    settings = await res1.json();
    state = await res2.json();
    updateElements();
  })
  .catch(error => {
    console.error(error);
  });  

  
  /* var xhr = new XMLHttpRequest();
  xhr.open("GET", "settings", false);
  xhr.send(null);
  if (xhr.status === 200) {
    settings = JSON.parse(xhr.responseText);
  }
  var xhr = new XMLHttpRequest();     
  xhr.open("GET", "state", false);
  xhr.send(null);
  if (xhr.status === 200) {
    state = JSON.parse(xhr.responseText);
    updateElements();
  } */
  
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

/* function BeforeSend(request) {
	
	$.blockUI({ css: { 
		border: 'none', 
		padding: '15px', 
		backgroundColor: '#000', 
		'-webkit-border-radius': '10px', 
		'-moz-border-radius': '10px', 
		opacity: .5, 
		color: '#fff' 
	} });
  
}

function Success(data, status, request) {
  
  setTimeout($.unblockUI, 0);
	// setTimeout(function() { 
  //   $.unblockUI({ 
  //       onUnblock: function(){ alert(data); } 
  //   }); 
  // }, 2000); 
  
}

function onError(request, status, error) {
	
	setTimeout(function() { 
    $.unblockUI({ 
        onUnblock: function(){ alert("error: " + request.responseText); } 
    }); 
  }, 2000);
} */

function postData(t, p) {
  /* 
  var e = new XMLHttpRequest;
  //e.addEventListener('loadstart', BeforeSend);
  //e.addEventListener('load', handleEvent);
  //e.addEventListener('loadend', Success);
  //e.addEventListener('progress', handleEvent);
  //e.addEventListener('error', Error);
  //e.addEventListener('abort', handleEvent);
  //e.timeout = 2000;
  e.open("POST", p, !0);
  e.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
  //e.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
  // fallbacks for IE and older browsers:
  //e.setRequestHeader("Expires", "Tue, 01 Jan 1980 1:00:00 GMT");
  //e.setRequestHeader("Pragma", "no-cache");
  //console.log(JSON.stringify(t));
  e.send(JSON.stringify(t));
 */
  
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
      $("#fan_lvl_" + i).attr("src", "level_" + i + "_on.svg");
    } else {
      $("#fan_lvl_" + i).attr("src", "level_" + i + "_off.svg");
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
    setTemp = setTimeout(function(){postData(state, "state");}, 1000);
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
    stepVState = {"stepv": true, "swingvert":false};
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
    turboState = {"turbo": true};
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
    stepVState = {"steph": true, "swinghor":false};
    postData(stepVState, "steph");
  }
}
