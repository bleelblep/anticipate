# Anticipate — shake-to-show details

Based on [Anticipate by Unruh Bros. Print Co.](https://github.com/unruh-bros-print-co/anticipate).

The quiet view shows only the time. Shake/flick to reveal the original left
column (date, weekday, daily high, current temperature, daily low and weather
icon) plus the battery gauge beneath the time. The gauge's upward marker and
percentage sit below its track. Both states retain the centred 144×168
footprint and four-pixel inset, including the original margins on larger
screens. The time smoothly shrinks to make room and expands when details hide.

## Clay settings

- Time numerals: Anticipate, Naive or Brutal.
- Details: Off, Always, or On wrist flick (default).
- Reveal duration: 3, 5 (default), 10, 15 or 30 seconds. Shake again to extend it.
- Date order and Celsius/Fahrenheit.
- Weather location: use phone GPS or enter a city/postal code with a country/state.
- Pebble Time 2 RGB backlight: enable custom colour and choose from 64 large swatches.
  Saving briefly previews the light. System brightness and timeout remain in
  control. Disabling restores the system tint. Tint is reapplied on focus return
  and backlight activation using the documented Light API.

Watch identity is obtained only after PebbleKit JS is ready or while opening
configuration. All Clay control types are registered at startup before platform
filtering, so the backlight controls cannot interrupt construction of the Save button.
Save has a 56px touch target and stays at the bottom as you scroll.

No health access is requested. Weather uses phone location and Open-Meteo, cached
for 30 minutes. Failed/unavailable data displays dashes; watch weather expires
after three hours. Incoming weather never cancels an active reveal. Sharp
movements other than wrist flicks may also trigger Pebble's tap/motion callback.

## Numeral artwork and licensing

Naive and Brutal are by [ir33k](https://github.com/ir33k). Their original GPL-2.0
PDC numeral files are included under `resources/naive` and `resources/brutal`.
Naive uses its original separate hour/minute shapes. Generated pixel rows are
scaled into this face's layout; upstream shadows and other styling are omitted.
Anticipate's original MIT notice is retained in `licenses/Anticipate-MIT.txt`.
The combined distribution is GPL-2.0; see LICENSE and licenses/NOTICE.md.

Corresponding source: https://github.com/bleelblep/anticipate

## Build and tests

Requires Pebble SDK 4.33.1, pebble-tool 5.0.40, and Node/npm. Run `npm ci`,
`pebble clean`, then `pebble build`. Output: `build/anticipate.pbw`.
Supported platforms: basalt, diorite, flint, emery and gabbro. RGB controls
require Time 2 firmware exposing `light_set_color_rgb888`,
`light_set_system_color` and the BacklightService.

Run `sh tests/run.sh` for host-side regression tests: three numeral styles,
all times, all battery percentages, animation bounds/reversal, repeated flicks,
settings, weather delivery during reveal, integer tuple widths, and real bundled Clay
Save/transport tests (offline, with simulated location failures and send retries).
Physical wrist detection, backlight colours and animation performance still
need verification on a real watch.

Generated artwork can be rebuilt with Pillow:
- `python tools/generate_time_digits.py`
- `python tools/generate_column.py`
- `python tools/generate_alt_digits.py`

## 1.5.0 reliability fixes

Restores the stock Rebble Clay colour picker, with no custom palette or CSS.
Settings and weather share a serialized AppMessage queue with three delivery
attempts. Saved preferences are resent when PebbleKit JS starts. Weather/location
exceptions no longer block saving preferences. Watch numeric tuples are decoded
according to their actual byte length, and malformed strings are rejected before
parsing. Watch logs report applied styles/colours and dropped messages.

Validation: SDK build for all five platforms; bundled JavaScript transport tests;
host C regression tests. Hardware RGB and the reported intermittent crash remain
unverified. The tuple-width bug and location-exception path are reproduced defects,
not a confirmed diagnosis of the user's physical-watch crash.

RGB implementation reference inspected:
https://github.com/brooks2564/rgb-backlight-test/blob/master/src/c/main.c
The RGB packing already matches that reference. This watchface keeps the normal
backlight timeout rather than the test app's continuously enabled light.
