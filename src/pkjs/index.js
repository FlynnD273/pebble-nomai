// Import the Clay package
var Clay = require('@rebble/clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

function getBatteryLevel() {
	if (navigator.getBattery) {
		navigator.getBattery().then(x => {
			dict = { "PhoneBattLevel": Math.floor(x.level * 100) };
			Pebble.sendAppMessage(dict,
				function(e) {
					console.log('Battery level sent successfully!', JSON.stringify(dict));
				},
				function(e) {
					console.log('Error sending battery level!');
				}
			);
		})
	}
}

getBatteryLevel();

Pebble.addEventListener('appmessage',
	function(e) {
		var dict = e.payload;

		if ('Action' in dict) {
			if (dict['Action'] == "GetPhoneBatt") {
				getBatteryLevel();
			}
		}
	}
);
