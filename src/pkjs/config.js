module.exports = function(platform) {
  var palette=[];
  for(var r=0;r<4;r++) for(var g=0;g<4;g++) {
    var row=[];
    for(var b=0;b<4;b++) row.push((r*85<<16)|(g*85<<8)|b*85);
    palette.push(row);
  }
  // Eight columns retain all 64 Pebble colours with larger touch targets.
  var grid=[];
  for(var i=0;i<palette.length;i+=2) grid.push(palette[i].concat(palette[i+1]));
  var config = [
    {type: 'heading', defaultValue: 'Anticipate'},
    {type: 'section', items: [{type: 'select', messageKey: 'TimeStyle', label: 'Time numerals', defaultValue: '0', options: [
      {label: 'Anticipate', value: '0'}, {label: 'Naive — ir33k', value: '1'}, {label: 'Brutal — ir33k', value: '2'}
    ]}]},
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
  config.push({type: 'section', items: [
    {type: 'heading', defaultValue: 'Weather location'},
    {type: 'toggle', messageKey: 'WeatherGPS', label: 'Use phone location', defaultValue: true},
    {type: 'input', messageKey: 'WeatherLocation', label: 'City or postal code', defaultValue: '', attributes: {type: 'text', placeholder: 'London, UK'}},
    {type: 'text', defaultValue: 'Turn off phone location to use the city above. Include a country or state to distinguish places with the same name.'}
  ]});
  if (platform === 'emery') {
    config.push({type: 'section', items: [
      {type: 'heading', defaultValue: 'Pebble Time 2 backlight'},
      {type: 'toggle', messageKey: 'CustomBacklight', label: 'Use custom RGB backlight', defaultValue: false},
      {type: 'color', messageKey: 'BacklightColor', label: 'Backlight colour', defaultValue: 'ffffff', sunlight: false, layout: grid},
      {type: 'text', defaultValue: 'Saving previews the backlight colour briefly. Your watch still controls brightness and how long the light stays on. All channels at zero gives no coloured light.'}
    ]});
  }
  config.push({type: 'submit', defaultValue: 'Save'});
  return config;
};
