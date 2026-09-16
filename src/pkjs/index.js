var Clay=require('@rebble/clay');
var config=require('./config');
var weather=require('./weather');
// Register every component before filtering the page for the connected watch.
var clay=new Clay(config('emery'),require('./custom'),{autoHandleEvents:false});
// Watch information is only valid inside ready/showConfiguration callbacks.
function configure() {
  var info=Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  clay.config=config(info && info.platform);
}
Pebble.addEventListener('ready',function() {configure();weather.request();});
Pebble.addEventListener('showConfiguration',function() {configure();Pebble.openURL(clay.generateUrl());});
Pebble.addEventListener('webviewclosed',function(e) {
  if(!e || !e.response || e.response==='CANCELLED') return;
  try {
    var settings=clay.getSettings(e.response);
    delete settings[require('message_keys').WeatherGPS];
    delete settings[require('message_keys').WeatherLocation];
    weather.request(true);
    Pebble.sendAppMessage(settings);
  }
  catch(error) {console.log('Settings could not be read');}
});
Pebble.addEventListener('appmessage',function(e) {
  if(e.payload && e.payload.REQUEST_WEATHER) weather.request();
});
