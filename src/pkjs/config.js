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
			}
		]
	},
	{
		"type": "submit",
		"defaultValue": "Save Settings"
	}
];
