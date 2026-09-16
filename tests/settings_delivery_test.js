// Exercise the shipped JS bundle and real Clay conversion, not a Clay mock.
const assert=require('assert'),fs=require('fs'),vm=require('vm');
const bundle=fs.readFileSync(process.argv[2]||'build/pebble-js-app.js','utf8');
const keys=require('../build/js/message_keys.json');
let handlers={},storage={},pending=[],calls=[],timers=[],logs=[],weatherCalls=0;
const context={console:{log:x=>logs.push(x),error:x=>logs.push(x)},
 setTimeout:fn=>{timers.push(fn);return timers.length;},clearTimeout:()=>{},
 localStorage:{getItem:k=>storage[k]||null,setItem:(k,v)=>storage[k]=v},
 navigator:{geolocation:{getCurrentPosition:()=>{weatherCalls++;throw Error('location unavailable');}}},
 Pebble:{addEventListener:(n,f)=>handlers[n]=f,getActiveWatchInfo:()=>({platform:'emery'}),openURL:()=>{},
 sendAppMessage:(data,ok,fail)=>{calls.push(JSON.parse(JSON.stringify(data)));pending.push({ok,fail});}}};
vm.runInNewContext(bundle,context);
handlers.ready(); // Location failure must not prevent future saves.
const values={TimeStyle:{value:'1'},BatteryMode:{value:'1'},BatterySeconds:{value:'10'},
 CustomBacklight:{value:true},BacklightColor:{value:0xaa0055},DateMonthFirst:{value:false},Fahrenheit:{value:true},
 WeatherGPS:{value:true},WeatherLocation:{value:'ignored on watch'}};
handlers.webviewclosed({response:encodeURIComponent(JSON.stringify(values))});
assert.equal(calls.length,1,'Settings must be sent even when geolocation throws');
assert.equal(calls[0][keys.TimeStyle],'1');assert.equal(calls[0][keys.CustomBacklight],1);
assert.equal(calls[0][keys.BacklightColor],0xaa0055);
assert(!Object.hasOwn(calls[0],keys.WeatherLocation));
pending.shift().fail({error:'busy'});assert.equal(timers.length,1);
timers.shift()();assert.equal(calls.length,2);assert.deepEqual(calls[0],calls[1]);
pending.shift().ok();assert(weatherCalls>=2);
handlers.ready();assert.equal(calls.length,3,'Saved settings must be resent after JS restart');
pending.shift().ok();
const before=calls.length;handlers.webviewclosed({response:'CANCELLED'});assert.equal(calls.length,before);
// Weather transport must serialize behind a settings packet.
handlers.webviewclosed({response:encodeURIComponent(JSON.stringify(values))});
const cache={TEMP_HI:20,TEMP_CUR:15,TEMP_LO:10,CONDITIONS:0,WEATHER_AT:Math.floor(Date.now()/1000),locationKey:'GPS'};
storage['anticipate-column-weather-v1']=JSON.stringify(cache);
handlers.appmessage({payload:{REQUEST_WEATHER:1}});
assert.equal(calls.length,before+1,'No weather send while settings awaits acknowledgment');
pending.shift().ok();assert.equal(calls.length,before+2);
assert.equal(calls[calls.length-1].TEMP_CUR,15);pending.shift().ok();
assert(logs.some(x=>x==='Settings delivered'));
console.log('PASS: real bundled Clay Save, colour/font payload, location exceptions, retries, restart replay and serialized weather.');
