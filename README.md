# Anticipate — time and battery

A modification of [Anticipate by Unruh Bros. Print Co.](https://github.com/unruh-bros-print-co/anticipate).

The original numeral artwork stays inside its centred 144 × 168 composition,
with four-pixel insets and unchanged outer margins on larger displays.
When the battery gauge appears, the time smoothly shrinks from 66 × 78 to
56 × 66 pixels per full-width digit. A 330 ms eased transition reveals the
thin gauge below it; an upward marker and percentage sit beneath the bar.
At low/high charge, the current percentage replaces the nearby endpoint label
to avoid overlapping text. Time follows the watch's 12/24-hour preference.

## Clay settings

- Battery bar: Off, Always, or On wrist flick (default).
- Display duration: 3, 5 (default), 10, 15, or 30 seconds.
- Pebble Time 2 only: custom backlight toggle and full-range red, green and
  blue sliders (0–255 each). Disabled uses the system colour.

Flick detection uses Pebble's standard accelerometer tap/motion callback;
other sharp movements can trigger it too. Another flick restarts the timer.
The time expands again when the bar hides. Animation and expiry timers stop
when the face loses focus or exits. The backlight tint is reapplied after
notifications; normal system brightness and illumination timing remain intact.
No weather, location or health access is requested.

## Build

Use Pebble SDK 4.33.1 (the RGB APIs must be present), Node/npm and pebble-tool.
Run `npm ci`, then `pebble build`. The result is `build/anticipate.pbw`.
RGB control on emery also requires firmware providing
`light_set_color_rgb888` and `light_set_system_color`.

Supported targets: basalt, diorite, flint, emery, gabbro. RGB settings and API
calls are restricted to emery (Pebble Time 2).

## Checks

Successfully compiled with Pebble SDK 4.33.1 for all five supported targets.
Physical watch verification is still pending.

Run `sh tests/run.sh` for host-side SDK-mock tests covering all times, gauge
levels, animation clipping, reversal, repeated flicks, settings validation,
focus changes, timer failure and RGB platform gating. They do not substitute
for physical wrist-motion, display-performance or RGB-light verification.

`src/c/time_digits.h` contains the original PNG pixels. Regenerate it with
`python tools/generate_time_digits.py` (Pillow required).

Original license and artwork attribution are retained in LICENSE.
