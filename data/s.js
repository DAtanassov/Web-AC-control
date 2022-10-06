var settings = {};

function getSettings() {
  $.ajax({
    type: 'GET',
    url: "settings",
    dataType: "json",
    beforeSend: BeforeSend,
    success: function(data, status, request) {
      if (!data) {
        return;
      }
      settings = data;
      updateElements();
      Success(data, status, request)
    },
    error: function(request, status, error) {
      console.log('error getting settings');
      onError(request, status, error)
    },
    timeout: 1000
    //,cache: false
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

function BeforeSend(request) {
	
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
  /*
  setTimeout(function() { 
      $.unblockUI({ 
          onUnblock: function(){ alert("Success"); } 
      }); 
    }, 2000);
    */
}

function onError(request, status, error) {
	
	setTimeout(function() { 
    $.unblockUI({ 
        onUnblock: function(){ alert("error: " + request.responseText); } 
    }); 
  }, 2000);
}

function postData(t, p) {
  var e = new XMLHttpRequest;
  e.addEventListener('loadstart', BeforeSend);
  //e.addEventListener('load', handleEvent);
  e.addEventListener('loadend', Success);
  //e.addEventListener('progress', handleEvent);
  e.addEventListener('error', Error);
  //e.addEventListener('abort', handleEvent);
  //e.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
  // fallbacks for IE and older browsers:
  //e.setRequestHeader("Expires", "Tue, 01 Jan 1980 1:00:00 GMT");
  //e.setRequestHeader("Pragma", "no-cache");
  e.open("POST", p, !0);
  e.setRequestHeader("Content-Type", "application/json");
  console.log(JSON.stringify(t));
  e.send(JSON.stringify(t));
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
