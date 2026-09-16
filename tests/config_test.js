const assert = require('assert');
const config = require('../src/pkjs/config');
const manifest = require('../package.json');
const collect = items => items.flatMap(i => i.items ? collect(i.items) : [i]);
const emery = collect(config('emery'));
for (const item of emery.filter(i => i.messageKey)) assert(manifest.pebble.messageKeys.includes(item.messageKey));
assert.strictEqual(emery.find(i=>i.messageKey==='TimeStyle').options.length,3);
for (const platform of ['basalt', 'diorite', 'flint', 'gabbro', undefined]) {
  assert(!collect(config(platform)).some(i => /Backlight/.test(i.messageKey || '')));
}
for (const item of emery.filter(i => i.type === 'slider')) {
  assert(item.min === 0 && item.max === 255 && item.step === 1 && item.defaultValue === 255);
}
assert(emery.find(i => i.messageKey === 'BatteryMode').defaultValue === '2');
assert(emery.find(i => i.messageKey === 'BatterySeconds').defaultValue === '5');
assert(emery.some(i=>i.type==='submit'));
assert(emery.some(i=>i.type==='color' && i.messageKey==='BacklightColor'));
assert(emery.some(i=>i.messageKey==='WeatherLocation'));
console.log('PASS: Clay keys, defaults and platform-specific RGB controls.');

const vm=require('vm'),fs=require('fs');
const handlers={},calls=[];let ready=false,watch={platform:'emery'},instance;
function MockClay(c) {this.config=c;instance=this;this.generateUrl=()=>'';}
const Pebble={addEventListener:(n,f)=>handlers[n]=f,getActiveWatchInfo:()=>{assert(ready,'Premature watch info access');return watch;},openURL:()=>{}};
vm.runInNewContext(fs.readFileSync(require.resolve('../src/pkjs/index.js'),'utf8'),{Pebble,console,require:n=>n==='@rebble/clay'?MockClay:n==='./config'?config:{request:()=>calls.push('weather')}});
ready=true;handlers.ready();assert(collect(instance.config).some(i=>i.messageKey==='BacklightColor'));
watch=null;handlers.showConfiguration();watch={platform:'emery'};handlers.showConfiguration();
assert(collect(instance.config).some(i=>i.messageKey==='BacklightColor'));
const weather=require('../src/pkjs/weather');
assert.strictEqual(weather.parse({},1),null);
assert.strictEqual(weather.parse({current:{temperature_2m:null},daily:{temperature_2m_max:[20],temperature_2m_min:[10]}},1),null);
assert.strictEqual(weather.condition(0,false),1);
assert.strictEqual(weather.condition(95,true),8);
console.log('PASS: deferred watch discovery, null-watch handling and weather validation.');

// Clay only registers controls present at construction, not later config replacements.
const Module=require('module'),originalLoad=Module._load;
Module._load=function(name) {return name==='message_keys'?{}:originalLoad.apply(this,arguments);};
const RealClay=require('../node_modules/@rebble/clay/dist/js/index.js');
const actual=new RealClay(config('emery'),null,{autoHandleEvents:false});
for(const platform of ['emery','basalt']) for(const item of collect(config(platform))) {
  assert(actual.components[item.type], 'Unregistered component '+item.type);
}
Module._load=originalLoad;
console.log('PASS: every platform control is registered before page construction.');
