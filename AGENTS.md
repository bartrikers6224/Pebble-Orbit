---
alwaysApply: true
---

## Project Overview

Orbit is a Pebble Round 2 (gabbro) watchface: digital time in the center, three concentric progress rings for hours, minutes, and seconds. Seconds update every 10 seconds.

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
src/c/main.c          Watchface drawing, 10s seconds timer, Clay persistence
src/pkjs/index.js     Clay bootstrap
src/pkjs/config.js    Color pickers for background, time, and three rings
```
