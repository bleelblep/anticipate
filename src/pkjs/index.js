var Clay = require('@rebble/clay');
var config = require('./config');
var info = Pebble.getActiveWatchInfo ? Pebble.getActiveWatchInfo() : {};
new Clay(config(info.platform));
