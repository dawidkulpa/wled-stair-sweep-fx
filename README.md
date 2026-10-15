# wled-stair-sweep-fx

External [WLED](https://github.com/wled/WLED) usermod (WLED 16.x) adding a one-shot **Smooth Sweep** effect for stair lights, plus build configuration for QuinLED Dig-Uno / Dig-Quad V3 boards. Replaces the former fork `dawidkulpa/WLED`: firmware is built from an unmodified official WLED tag.

## Effect

`Smooth Sweep` (and the alias `Smooth Sweep Fill`, kept for old presets) sweeps once from **color 2** (background) to **color 1** (primary) with a soft gradient on the moving front, then stays filled with color 1.

| Control | Meaning |
|---|---|
| Speed | sweep duration: 255 = 0.75 s, 0 = 39 s |
| Gradient width | 1 LED up to half the segment |
| Color 1 / Color 2 | target color / starting (background) color |

A fresh sweep (over color 2) starts when the effect is selected, when the light is switched on, and when the segment is switched on.

When color 1, color 2 or the segment direction (`rev`) change while the effect is already selected, for example a "turn off" preset arriving in the middle of a "turn on" sweep, the new sweep paints over the frame currently shown instead of jumping to the new colors: a half-lit staircase is taken back from the side the new sweep comes from. The front starts at the first LED that still needs to change, so there is no idle run over LEDs that are already right. Re-sending identical colors and direction does not restart the sweep (WLED does not reset an effect that is already selected).

Stairs setup this was written for: presets "Turn on" / "Turn on reverse" and "Turn off B2T" / "Turn off T2B" use this effect with `rev` for the direction and colors swapped for turning off, each wrapped in a short playlist that ends on a Solid preset.

## Build

Requires PlatformIO and Node.js.

```sh
./build.sh v16.0.1          # usermod from the tag pinned in platformio_override.ini
./build.sh v16.0.1 --local  # usermod from this checkout (development)
```

Firmware files land in `.build/out-<tag>/`. To update WLED later, run the script with the new tag. To release a usermod change, tag this repo and bump the tag in `platformio_override.ini`.

Flash over OTA from the WLED UI (Config → Security & Updates → Manual OTA update). Effect IDs are assigned at boot, so presets that reference these effects by number must be re-saved after an upgrade; references by name keep working.
