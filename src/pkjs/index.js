var Clay=require('@rebble/clay');
var config=require('./config');
var weather=require('./weather');
var clay=new Clay(config(),null,{autoHandleEvents:false});
// Watch information is only valid inside ready/showConfiguration callbacks.
function configure() {
  var info=Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : null;
  clay.config=config(info && info.platform);
}
Pebble.addEventListener('ready',function() {configure();weather.request();});
Pebble.addEventListener('showConfiguration',function() {configure();Pebble.openURL(clay.generateUrl());});
Pebble.addEventListener('webviewclosed',function(e) {
  if(!e || !e.response || e.response==='CANCELLED') return;
  try {Pebble.sendAppMessage(clay.getSettings(e.response),function() {weather.request();});}
  catch(error) {console.log('Settings could not be read');}
});
Pebble.addEventListener('appmessage',function(e) {
  if(e.payload && e.payload.REQUEST_WEATHER) weather.request();
});
