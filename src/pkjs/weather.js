// Open-Meteo values are cached in Celsius; the watch applies the user's unit.
var busy = false;
var CACHE = 'anticipate-column-weather-v1';
function condition(code, day) {
  if (code === 0) return day ? 0 : 1;
  if (code === 1 || code === 2) return day ? 2 : 3;
  if (code === 3) return 4;
  if (code === 45 || code === 48) return 9;
  if (code >= 51 && code <= 57) return 5;
  if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return 6;
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return 7;
  if (code >= 95 && code <= 99) return 8;
  return -1;
}
function parse(json, now) {
  var c=json.current, d=json.daily;
  if (!c || !d || !d.temperature_2m_max || !d.temperature_2m_min) return null;
  var values=[d.temperature_2m_max[0], c.temperature_2m, d.temperature_2m_min[0]];
  if (!values.every(function(n) {return typeof n==='number' && isFinite(n) && n>=-100 && n<=100;})) return null;
  return {TEMP_HI: Math.round(values[0]), TEMP_CUR: Math.round(values[1]),
    TEMP_LO: Math.round(values[2]), CONDITIONS: condition(c.weather_code,c.is_day), WEATHER_AT: now};
}
function request() {
  if (busy) return;
  var now=Math.floor(Date.now()/1000), cache;
  try {cache=JSON.parse(localStorage.getItem(CACHE));} catch(e) {cache=null;}
  if (cache && now>=cache.WEATHER_AT && now-cache.WEATHER_AT<1800) {
    Pebble.sendAppMessage(cache); return;
  }
  busy=true;
  navigator.geolocation.getCurrentPosition(function(pos) {
    var xhr=new XMLHttpRequest();
    xhr.open('GET','https://api.open-meteo.com/v1/forecast?latitude='+pos.coords.latitude+
      '&longitude='+pos.coords.longitude+'&daily=temperature_2m_max,temperature_2m_min'+
      '&current=temperature_2m,is_day,weather_code&timezone=auto&forecast_days=1&temperature_unit=celsius');
    xhr.timeout=15000;
    xhr.onload=function() {
      busy=false;
      if(xhr.status!==200) return;
      try {
        var data=parse(JSON.parse(xhr.responseText),Math.floor(Date.now()/1000));
        if(data) {localStorage.setItem(CACHE,JSON.stringify(data));Pebble.sendAppMessage(data);}
      } catch(e) {console.log('Weather response unavailable');}
    };
    xhr.onerror=xhr.ontimeout=function() {busy=false;};
    xhr.send();
  },function() {busy=false;},{timeout:15000,maximumAge:60000});
}
module.exports={request:request,parse:parse,condition:condition};
