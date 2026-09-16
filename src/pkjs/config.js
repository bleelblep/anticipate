module.exports = function(platform) {
  var config = [
    {type: 'select', messageKey: 'TimeStyle', label: 'Time numerals', defaultValue: '0', options: [
      {label: 'Anticipate', value: '0'}, {label: 'Naive — ir33k', value: '1'}, {label: 'Brutal — ir33k', value: '2'}
    ]},
    {type: 'heading', defaultValue: 'Anticipate'},
    {type: 'section', items: [
      {type: 'heading', defaultValue: 'Shake-to-show details'},
      {type: 'select', messageKey: 'BatteryMode', label: 'Show left column and battery', defaultValue: '2', options: [
        {label: 'Off', value: '0'}, {label: 'Always', value: '1'},
        {label: 'On wrist flick', value: '2'}
      ]},
      {type: 'select', messageKey: 'BatterySeconds', label: 'Show after flick for', defaultValue: '5', options: [
        {label: '3 seconds', value: '3'}, {label: '5 seconds', value: '5'},
        {label: '10 seconds', value: '10'}, {label: '15 seconds', value: '15'},
        {label: '30 seconds', value: '30'}
      ]},
      {type: 'text', defaultValue: 'Reveal the original date, weekday and weather column together with the battery bar. Shake again to keep them visible longer.'}
    ]}
  ];
  config.push({type: 'section', items: [
    {type: 'heading', defaultValue: 'Date and weather'},
    {type: 'toggle', messageKey: 'DateMonthFirst', label: 'Month before day', defaultValue: false},
    {type: 'toggle', messageKey: 'Fahrenheit', label: 'Use Fahrenheit', defaultValue: false}
  ]});
  if (platform === 'emery') {
    config.push({type: 'section', items: [
      {type: 'heading', defaultValue: 'Pebble Time 2 backlight'},
      {type: 'toggle', messageKey: 'CustomBacklight', label: 'Use custom RGB backlight', defaultValue: false},
      {type: 'slider', messageKey: 'BacklightRed', label: 'Red', defaultValue: 255, min: 0, max: 255, step: 1},
      {type: 'slider', messageKey: 'BacklightGreen', label: 'Green', defaultValue: 255, min: 0, max: 255, step: 1},
      {type: 'slider', messageKey: 'BacklightBlue', label: 'Blue', defaultValue: 255, min: 0, max: 255, step: 1},
      {type: 'text', defaultValue: 'Saving previews the backlight colour briefly. Your watch still controls brightness and how long the light stays on. All channels at zero gives no coloured light.'}
    ]});
  }
  config.push({type: 'submit', defaultValue: 'Save'});
  return config;
};
