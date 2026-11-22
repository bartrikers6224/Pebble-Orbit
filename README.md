# Orbit

A Pebble Round 2 watchface: large digital time in the center, three concentric rings around it.

![Orbit on Pebble Round 2](screenshots/screenshot_gabbro.png)

- **Outer ring** — hours (full circle at 12 hours in AM/PM, 24 hours in 24h mode)
- **Middle ring** — minutes
- **Inner ring** — seconds, at 1, 10, or 30 second steps (Clay setting)

Tap the gear next to Orbit in the Pebble app to set colors, the seconds interval, and idle sleep.

Built for **Pebble Round 2** (`gabbro`) on Pebble SDK 4.44.

## Install on your watch

The built watchface is **`orbit.pbw`** in this repo. Send that file to your phone (AirDrop, email, Drive), then open it with the Pebble app while the Round 2 is connected.

- **iPhone:** Files, Mail, or AirDrop → tap `orbit.pbw` → open in **Pebble**.
- **Android:** Files / Downloads → tap `orbit.pbw` → open with **Pebble**. If Pebble is not listed, install [Sideload Helper by Rebble](https://help.rebble.io/sideloading/) first.

Orbit shows up in the locker as sideloaded. Start it from **Apps → Watchfaces**. Tap the gear for settings.

To install from a computer with the SDK instead:

```bash
pebble login
pebble install --cloudpebble
```

(Enable **Dev Connection** first: Pebble app → Devices → ⋮ on the watch.)

## Build locally

You need the [Pebble SDK](https://developer.repebble.com/sdk) (`pebble-tool` 5.1+ and SDK 4.44).

```bash
pebble sdk install latest
pebble build
pebble install --emulator gabbro
```

On a machine without a display, add `--vnc` to emulator commands.

Open the settings page against the emulator with:

```bash
pebble emu-app-config --emulator gabbro
```

## Battery

Time digits and the hour/minute rings update once a minute. The seconds ring is driven by an `AppTimer` aligned to the interval you pick — 1, 10, or 30 seconds. There is no `SECOND_UNIT` tick subscription.

**Sleep when idle** (off by default) stops those timers while the watch is charging, or after about five minutes with no motion. The last frame stays on the e-paper display. Shake or tap to wake. Use this if you want a 1-second seconds ring while worn; the watch still sleeps on the charger or a table.
