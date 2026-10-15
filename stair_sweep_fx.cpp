#include "wled.h"

// One-shot sweep from the background color (color 2) to the primary color (color 1)
// with a soft gradient on the moving front. After the sweep the segment stays filled
// with the primary color.
//
// The sweep (re)starts when:
//  - the effect is selected (segment reset),
//  - the light is switched on (global brightness 0 -> >0, detected by the usermod below),
//  - the segment is switched on.

// Incremented by the usermod each time the light is switched on.
static uint16_t sweepTrigger = 0;

static void mode_smooth_sweep(void) {
  if (SEGLEN <= 1) {
    SEGMENT.fill(SEGCOLOR(0));
    return;
  }

  // step = sweep start (millis), aux0 = last seen sweepTrigger, aux1 = segment on state in the previous frame
  const bool segOn = SEGMENT.on;
  if (SEGENV.call == 0 || SEGENV.aux0 != sweepTrigger || (segOn && !SEGENV.aux1)) {
    SEGENV.step = millis(); // real time, not strip.now: a timebase sync must not jump the sweep
    SEGENV.aux0 = sweepTrigger;
  }
  SEGENV.aux1 = segOn;

  const uint32_t color  = SEGCOLOR(0);
  const uint32_t color2 = SEGCOLOR(1);

  const uint32_t cycleTime = 750 + (255 - SEGMENT.speed) * 150;
  const uint32_t elapsed = millis() - SEGENV.step;
  if (elapsed >= cycleTime) {
    // sweep finished: keep repainting so color changes still show up
    SEGMENT.fill(color);
    return;
  }

  unsigned gradWidth = map(SEGMENT.intensity, 0, 255, 1, SEGLEN / 2);
  if (gradWidth < 1) gradWidth = 1;

  // Positions in 1/256 pixel so the front glides instead of jumping a whole LED per step.
  // The sweep runs past the strip end so the gradient fully clears the last LED.
  const uint32_t grad     = gradWidth << 8;
  const uint32_t sweepEnd = (SEGLEN - 1 + gradWidth) << 8;
  const uint32_t front    = (uint64_t)elapsed * sweepEnd / cycleTime;

  for (unsigned i = 0; i < SEGLEN; i++) {
    const uint32_t pos = i << 8;
    if (pos > front) {
      SEGMENT.setPixelColor(i, color2);
      continue;
    }
    const uint32_t distanceFromFront = front - pos;
    if (distanceFromFront >= grad) {
      SEGMENT.setPixelColor(i, color);
    } else {
      // gradient zone: color2 at the leading edge, color deeper in
      uint8_t fadeAmount = (255 * distanceFromFront) / grad;
      if (fadeAmount < 20) fadeAmount = 20; // avoid low-PWM color distortion on LEDs
      SEGMENT.setPixelColor(i, color_blend(color2, color, fadeAmount));
    }
  }
}

static const char _data_FX_SMOOTH_SWEEP[] PROGMEM = "Smooth Sweep@!,Gradient width;!,!;";

// Alias of Smooth Sweep, kept for preset compatibility
static void mode_smooth_sweep_fill(void) {
  mode_smooth_sweep();
}

static const char _data_FX_SMOOTH_SWEEP_FILL[] PROGMEM = "Smooth Sweep Fill@!,Gradient width;!,!;";

class StairSweepFx : public Usermod {
  uint8_t lastBri = 0;

 public:
  void setup() override {
    strip.addEffect(255, &mode_smooth_sweep, _data_FX_SMOOTH_SWEEP);
    strip.addEffect(255, &mode_smooth_sweep_fill, _data_FX_SMOOTH_SWEEP_FILL);
  }

  void loop() override {
    if (bri && !lastBri) sweepTrigger++; // light switched on: replay the sweep
    lastBri = bri;
  }
};

static StairSweepFx stair_sweep_fx_instance;
REGISTER_USERMOD(stair_sweep_fx_instance);
