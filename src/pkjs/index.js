var Clay=require('@rebble/clay');
var config=require('./config');
var weather=require('./weather');
var transport=require('./transport');
var keys=require('message_keys');
var clay=new Clay(config('emery'),null,{autoHandleEvents:false});
function configure() {
  var info=Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  clay.config=config(info && info.platform);
}
function requestWeather(force) {
  try {weather.request(force);}
  catch(error) {console.log('Weather request failed: '+String(error));}
}
function sendSettings(raw) {
  // Keep location on the phone and send only known watch preferences.
  var allowed=['TimeStyle','BatteryMode','BatterySeconds','CustomBacklight',
    'BacklightRed','BacklightGreen','BacklightBlue','BacklightColor','DateFormat','Fahrenheit',
    'BackgroundColor','DigitColor','DetailColor'];
  // Carry the old "month before day" toggle over until a date format is saved.
  if(!Object.prototype.hasOwnProperty.call(raw,'DateFormat') && Object.prototype.hasOwnProperty.call(raw,'DateMonthFirst')) {
    var first=raw.DateMonthFirst;
    raw.DateFormat=(first && typeof first==='object' ? first.value : first) ? '1' : '0';
  }
  var filtered={};
  allowed.forEach(function(key){if(Object.prototype.hasOwnProperty.call(raw,key)) filtered[key]=raw[key];});
  var settings=Clay.prepareSettingsForAppMessage(filtered);
  if(Object.keys(settings).length) transport.send('Settings',settings,function(){requestWeather(true);});
  else requestWeather();
}
Pebble.addEventListener('ready',function() {
  configure();
  var saved={};
  try {saved=JSON.parse(localStorage.getItem('clay-settings'))||{};}
  catch(error) {console.log('Saved settings invalid: '+String(error));}
  sendSettings(saved);
});
Pebble.addEventListener('showConfiguration',function(){configure();Pebble.openURL(clay.generateUrl());});
Pebble.addEventListener('webviewclosed',function(e) {
  if(!e || !e.response || e.response==='CANCELLED') return;
  try {sendSettings(clay.getSettings(e.response,false));}
  catch(error) {console.log('Settings could not be read: '+String(error));}
});
Pebble.addEventListener('appmessage',function(e) {
  if(e.payload && (e.payload.REQUEST_WEATHER || e.payload[keys.REQUEST_WEATHER])) requestWeather();
});
