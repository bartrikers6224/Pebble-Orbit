---
alwaysApply: true
---

## Project Overview

Orbit is a Pebble Round 2 (gabbro) watchface: digital time in the center, three concentric progress rings for hours, minutes, and seconds. Seconds update on an AppTimer at 1s, 10s, 30s, or Adaptive (1s while the backlight is on or for 10s after a flick, otherwise 10s). Optional idle sleep stops updates while charging or after ~5 minutes still.

## Supported Platforms

- gabbro (Pebble Round 2, 260x260 round color)

## Commands

```bash
pebble build
pebble install --emulator gabbro
pebble screenshot --scale 6 --no-open screenshot_gabbro.png
```

Headless environments need `--vnc` on every emulator command:

```bash
pebble install --emulator gabbro --vnc
pebble screenshot --vnc --emulator gabbro --scale 6 --no-open screenshot_gabbro.png
```

Clay settings: `pebble emu-app-config --emulator gabbro`

## Project Structure

```
src/c/main.c          Watchface drawing, AppTimer seconds, idle sleep, Clay persistence
src/pkjs/index.js     Clay bootstrap
src/pkjs/config.js    Color pickers, seconds interval (including Adaptive), sleep-when-idle toggle
screenshots/gabbro_*.png  260x260 appstore shots: Midnight, Dusk, Paper
```
