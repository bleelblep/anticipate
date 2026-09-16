#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for platform in normal emery; do
  flag=''
  if [ "$platform" = emery ]; then flag='-DPBL_PLATFORM_EMERY -DPBL_RGB_BACKLIGHT'; fi
  cc -std=c99 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -Wl,--gc-sections $flag -Itests tests/watchface_test.c -o /tmp/anticipate-test
  /tmp/anticipate-test
done
node tests/config_test.js
node tests/weather_location_test.js

node tests/settings_delivery_test.js
