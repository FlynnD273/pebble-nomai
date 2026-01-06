const gaugeOptions = ["Watch battery level", "Phone battery level", "24-hour progress", "24-hour progress (inverted)", "Temperature (% range)", "Rain forecast"].map((x, i) => { return { label: x, value: i + "" } });
module.exports = [
	{
		"type": "heading",
		"defaultValue": "App Configuration"
	},
	{
		"type": "section",
		"items": [
			{
				"type": "slider",
				"messageKey": "ZoomInDur",
				"label": "Zoom in duration (ms)",
				"min": 0,
				"max": 10000,
				"step": 500,
				"defaultValue": 1000
			},
			{
				"type": "slider",
				"messageKey": "ZoomOutDur",
				"label": "Zoom out duration (ms)",
				"min": 0,
				"max": 10000,
				"step": 500,
				"defaultValue": 2000
			},
			{
				"type": "slider",
				"messageKey": "ZoomPauseDur",
				"label": "Duration to show time for (ms)",
				"min": 0,
				"max": 10000,
				"step": 500,
				"defaultValue": 5000
			},
			{
				"type": "select",
				"messageKey": "UpperGauge",
				"label": "Upper gauge information",
				"options": gaugeOptions,
				"defaultValue": gaugeOptions[1].value
			},
			{
				"type": "select",
				"messageKey": "LowerGauge",
				"label": "Lower gauge information",
				"options": gaugeOptions,
				"defaultValue": gaugeOptions[0].value
			}
		]
	},
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Weather settings"
			},
			{
				"type": "toggle",
				"messageKey": "WUseLoc",
				"label": "Use phone GPS (overrides manual coordinates)",
				"defaultValue": true
			},
			{
				"type": "text",
				"defaultValue": "Manual coordinates"
			},
			{
				"type": "input",
				"messageKey": "WLocN",
				"label": "°N",
				"defaultValue": "25"
			},
			{
				"type": "input",
				"messageKey": "WLocE",
				"label": "°E",
				"defaultValue": "-71"
			},
			{
				"type": "slider",
				"messageKey": "WTempMin",
				"label": "Minimum temp (°C)",
				"min": -50,
				"max": 70,
				"step": 1,
				"defaultValue": 0
			},
			{
				"type": "slider",
				"messageKey": "WTempMax",
				"label": "Maximum temp (°C)",
				"min": -50,
				"max": 70,
				"step": 1,
				"defaultValue": 40
			},
			{
				"type": "slider",
				"messageKey": "WPrecipOffset",
				"label": "Rain forecast lookahead (hours)",
				"min": 0,
				"max": 12,
				"step": 1,
				"defaultValue": 2
			},
		]
	},
	{
		"type": "submit",
		"defaultValue": "Save Settings"
	}
];
