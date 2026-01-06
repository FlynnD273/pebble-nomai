const Clay = require("@rebble/clay");
const Keys = require('message_keys');

var settings = {
	weather: {
		useLoc: false,
		lat: "0",
		lon: "0",
		tempMin: 0,
		tempMax: 0,
		precipOffset: 0,
	}
}

function saveSettings() {
	localStorage.setItem("settings", JSON.stringify(settings));
}
function loadSettings() {
	try {
		settings = JSON.parse(localStorage.getItem("settings")) || settings;
	}
	catch (e) { }
}
loadSettings();
saveSettings();

const clayConfig = require("./config");
const clay = new Clay(clayConfig, null, { autoHandleEvents: false });

function getBatteryLevel() {
	if (navigator.getBattery) {
		navigator.getBattery().then(x => {
			dict = { "PhoneBattLevel": Math.floor(x.level * 100) };
			Pebble.sendAppMessage(dict, null,
				function(e) {
					console.warn("Error sending battery level!", e);
				}
			);
		})
	}
}

function updateAll() {
	getWeather();
	getBatteryLevel();
}

Pebble.addEventListener("ready",
	function(_e) {
		updateAll();
	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		const dict = e.payload;
		if ("Action" in dict) {
			switch (dict["Action"]) {
				case "Battery":
					getBatteryLevel();
					break;
				case "Weather":
					getWeather();
					break;
			}
		}
	}
);

// Unused. I thought I needed it but I actually don't
function findConfigItem(config, key) {
	for (let i = 0; i < config.length; i++) {
		const item = config[i];
		if (item.messageKey && item.messageKey == key) {
			return item;
		}
		if (item.items) {
			const itemRet = findConfigItem(item.items, key);
			if (itemRet) {
				return itemRet;
			}
		}
	}
	return null;
}

Pebble.addEventListener('showConfiguration', function(_e) {
	const settingsString = localStorage.getItem("clay-settings");
	if (settingsString) {
		const claySettings = JSON.parse(settingsString);
		if (claySettings.WLocN == 25 && claySettings.WLocN == -71) {
			claySettings.WLocN = settings.weather.lat;
			claySettings.WLocE = settings.weather.lon;
			localStorage.setItem("clay-settings", JSON.stringify(claySettings));
		}
	}
	Pebble.openURL(clay.generateUrl());
});

function getKey(dict, key, setting, prop) {
	if (key in dict)
		setting[prop] = dict[key];
}

Pebble.addEventListener("webviewclosed", function(e) {
	if (e && !e.response) { return; }

	var dict = clay.getSettings(e.response);
	getKey(dict, Keys.WLocN, settings.weather, "lat");
	getKey(dict, Keys.WLocE, settings.weather, "lon");
	getKey(dict, Keys.WUseLoc, settings.weather, "useLoc");
	getKey(dict, Keys.WTempMin, settings.weather, "tempMin");
	getKey(dict, Keys.WTempMax, settings.weather, "tempMax");
	getKey(dict, Keys.WPrecipOffset, settings.weather, "precipOffset");

	saveSettings();

	Pebble.sendAppMessage(dict, null, function(e) {
		console.warn("Failed to send config data!", e);
	});
	updateAll();
});

// Adapted from https://github.com/HarrisonAllen/SheikahSlate_PebbleWatchface/
function xhrRequest(url, type, callback) {
	const xhr = new XMLHttpRequest();
	xhr.addEventListener("load", () => callback(xhr.response));
	xhr.responseType = "json";
	xhr.open(type, url);
	xhr.send();
};

function locationSuccess(pos) {
	if (pos) {
		settings.weather.lat = pos.coords.latitude;
		settings.weather.lon = pos.coords.longitude;
		saveSettings();
	}
	else {
		loadSettings();
	}
	if (!settings.weather.lat && !settings.weather.lon) { console.warn("No location set :("); return; }

	const url = `https://api.open-meteo.com/v1/forecast?latitude=${settings.weather.lat}&longitude=${settings.weather.lon}&timezone=auto&hourly=precipitation_probability&current=temperature_2m,apparent_temperature&forecast_days=2`;
	xhrRequest(url, "GET",
		function(res) {
			if (!res) {
				console.warn("Weather response is empty");
				return;
			}

			const temp = res.current.temperature_2m;
			const time = res.current.time;
			const curr_hour = time.substring(time.length - "00:00".length, time.length - ":00".length);
			const precip = Math.max(...res.hourly.precipitation_probability.slice(curr_hour, curr_hour + settings.weather.precipOffset));

			var dictionary = {
				"WTemp": (temp * 1 - settings.weather.tempMin) * (1 * settings.weather.tempMax - settings.weather.tempMin),
				"WPrecip": precip
			};
			Pebble.sendAppMessage(dictionary, null,
				function(e) {
					console.warn("Error sending weather info to Pebble!", e);
				}
			);
		}
	);
}

function locationError(err) {
	console.warn("Error requesting location!", err);
	locationSuccess(null);
}

function getWeather() {
	if (settings.weather.useLoc) {
		navigator.geolocation.getCurrentPosition(
			locationSuccess,
			locationError,
			{ timeout: 15000, maximumAge: 60000 }
		);
	} else {
		locationSuccess(null);
	}
}
