// vim:expandtab:ts=2:softtabstop=2:shiftwidth=2
var Clay = require('clay');
var clayConfig = require('config.json');
var clay = new Clay(clayConfig, null, {autoHandleEvents: false});

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var formify = function(data) {
  var params = [], i = 0;
  for (var name in data) {
    params[i++] = encodeURI(name) + '=' + encodeURI(data[name]);
  }
  return params.join('&');
};

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if(e && !e.response) { return; }
  clay.getSettings(e.response);
  updateTraffic();
});

// Pull traffic stats from Google Maps API.
var updateTraffic = function() {
  // Check to see if there's configuration set.
  var config = localStorage.getItem('clay-settings');
  if(!config) {
    console.log('No config found, so not updating.');
  }

  // Make sure there's locations set.
  var settings = JSON.parse(config);

  navigator.geolocation.getCurrentPosition(function(pos) {
    destinations = settings.l1lat+','+settings.l1long+'|'+settings.l2lat+','+settings.l2long;
		var params = {
			key: settings.apikey,
			departure_time: 'now',
			origins: pos.coords.latitude + ',' + pos.coords.longitude,
      destinations: destinations,
		};
    var url = 'https://maps.googleapis.com/maps/api/distancematrix/json?' + formify(params);
    console.log(url);

		xhrRequest(url, 'GET', function(responseText){
			var json = JSON.parse(responseText);

			// Convert seconds to minutes for display.
			var data = json['rows'][0]['elements'];
			var l1_minutes = Math.round(data[0]['duration_in_traffic']['value'] / 60);
			var l2_minutes = Math.round(data[1]['duration_in_traffic']['value'] / 60);

			// Send information to the watch.
			Pebble.sendAppMessage({0: l1_minutes, 1: l2_minutes}, function() {
				console.log('Message sent!');
			}, function(e) {
				console.log('Message failed: ' + JSON.stringify(e));
			});
		});
  }, function(err) {
    console.log('Location services not available.');
  }, {timeout: 15000, maximumAge: 60000, enableHighAccuracy: true});
};

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    // Make sure the traffic is pushed out at app initialization.
    updateTraffic();
    setInterval(updateTraffic, (1000 * 60 * 30));
  }
);
