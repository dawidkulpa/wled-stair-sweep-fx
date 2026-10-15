# wled-stair-sweep-fx

External [WLED](https://github.com/wled/WLED) usermod (WLED 16.x) adding a one-shot **Smooth Sweep** effect for stair lights, plus build configuration for QuinLED Dig-Uno / Dig-Quad V3 boards. Replaces the former fork `dawidkulpa/WLED`: firmware is built from an unmodified official WLED tag.

## Effect

`Smooth Sweep` (and the alias `Smooth Sweep Fill`, kept for old presets) sweeps once from **color 2** (background) to **color 1** (primary) with a soft gradient on the moving front, then stays filled with color 1.

| Control | Meaning |
|---|---|
| Speed | sweep duration: 255 = 0.75 s, 0 = 39 s |
| Gradient width | 1 LED up to half the segment |
| Color 1 / Color 2 | target color / starting (background) color |

The sweep restarts when the effect is selected, when the light is switched on, and when the segment is switched on. Re-sending the same effect while the light is already on does not restart it (WLED does not reset an effect that is already selected).

## Build

Requires PlatformIO and Node.js.

```sh
./build.sh v16.0.1          # usermod from the tag pinned in platformio_override.ini
./build.sh v16.0.1 --local  # usermod from this checkout (development)
```

Firmware files land in `.build/out-<tag>/`. To update WLED later, run the script with the new tag. To release a usermod change, tag this repo and bump the tag in `platformio_override.ini`.

Flash over OTA from the WLED UI (Config → Security & Updates → Manual OTA update). Effect IDs are assigned at boot, so presets that reference these effects by number must be re-saved after an upgrade; references by name keep working.
