# Anticipate — time only

A time-only modification of [Anticipate by Unruh Bros. Print Co.](https://github.com/unruh-bros-print-co/anticipate).

Keeps the original stacked numeral artwork, right alignment, leading zeros,
and the watch's 12/24-hour preference. The face retains its centred 144 × 168
composition on every supported platform, including the original outer margins
on larger screens. Digits expand into the removed left-hand complications and
bottom timeline, with a four-pixel inset and four-pixel gaps. Each full-width
digit is now 66 × 78 rather than 46 × 71.

Date, steps, weather, seconds, timeline, phone configuration, location access,
and motion handling have been removed. The display updates once per minute.

`src/c/time_digits.h` encodes the original numeral PNGs as monochrome rows.
To regenerate it, install Pillow and run `python tools/generate_time_digits.py`.
The watch draws scaled pixel runs directly, preserving crisp edges.

Supported platforms: basalt, diorite, flint, emery, gabbro.

Validation: layout bounds and numeral encoding checked locally. A Pebble SDK
build and emulator/device verification are still required; the SDK was not
available in the editing environment.

Original license and artwork attribution are retained in LICENSE.
