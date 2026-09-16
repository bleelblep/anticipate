const assert = require('assert');
const config = require('../src/pkjs/config');
const manifest = require('../package.json');
const collect = items => items.flatMap(i => i.items ? collect(i.items) : [i]);
const emery = collect(config('emery'));
assert.deepStrictEqual(emery.filter(i => i.messageKey).map(i => i.messageKey), manifest.pebble.messageKeys);
for (const platform of ['basalt', 'diorite', 'flint', 'gabbro', undefined]) {
  assert(!collect(config(platform)).some(i => /Backlight/.test(i.messageKey || '')));
}
for (const item of emery.filter(i => i.type === 'slider')) {
  assert(item.min === 0 && item.max === 255 && item.step === 1 && item.defaultValue === 255);
}
assert(emery.find(i => i.messageKey === 'BatteryMode').defaultValue === '2');
assert(emery.find(i => i.messageKey === 'BatterySeconds').defaultValue === '5');
console.log('PASS: Clay keys, defaults and platform-specific RGB controls.');
