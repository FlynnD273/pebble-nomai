const gaugeOptions = [{ label: "Watch battery level", value: "0" }, { label: "Phone battery level", value: "1" }];
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
		"type": "submit",
		"defaultValue": "Save Settings"
	}
];
