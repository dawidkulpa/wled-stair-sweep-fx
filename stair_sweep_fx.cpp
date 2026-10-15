#include "wled.h"

// One-shot sweep to the primary color (color 1) with a soft gradient on the moving front.
// After the sweep the segment stays filled with the primary color.
//
// A fresh sweep starts over the background color (color 2) when:
//  - the effect is selected (segment reset),
//  - the light is switched on (global brightness 0 -> >0, detected by the usermod below),
//  - the segment is switched on.
//
// When color 1, color 2 or the segment direction change while the effect is already running
// (e.g. a "turn off" preset arrives in the middle of a "turn on" sweep), the new sweep starts
// over the frame currently shown, so a half-finished sweep is taken back instead of the strip
// jumping to the other colors. The front starts at the first LED that still needs to change.

// Incremented by the usermod each time the light is switched on.
static uint16_t sweepTrigger = 0;

struct SweepData {
  uint32_t color;     // target color 1 of the running sweep
  uint32_t color2;    // target color 2 of the running sweep
  uint16_t trigger;   // last seen sweepTrigger
  uint16_t startPx;   // first LED the front starts from
  bool     reverse;   // segment direction of the running sweep
  bool     segOn;     // segment on state in the previous frame
};

static void mode_smooth_sweep(void) {
  // Target colors, not SEGCOLOR(): WLED fades SEGCOLOR between old and new colors for the
  // transition time, while this effect is the transition itself.
  const uint32_t color  = SEGMENT.colors[0];
  const uint32_t color2 = SEGMENT.colors[1];

  if (SEGLEN <= 1 || !SEGMENT.allocateData(sizeof(SweepData) + SEGLEN * sizeof(uint32_t))) {
    SEGMENT.fill(color);
    return;
  }
  SweepData *d = reinterpret_cast<SweepData *>(SEGMENT.data);
  uint32_t *startFrame = reinterpret_cast<uint32_t *>(SEGMENT.data + sizeof(SweepData)); // frame the sweep paints over
  const bool segOn = SEGMENT.on;

  const bool fresh    = SEGENV.call == 0 || d->trigger != sweepTrigger || (segOn && !d->segOn);
  const bool retarget = !fresh && (d->color != color || d->color2 != color2 || d->reverse != SEGMENT.reverse);

  if (fresh) {
    for (unsigned i = 0; i < SEGLEN; i++) startFrame[i] = color2;
  } else if (retarget) {
    // The pixel buffer holds the last frame in effect order; reverse is applied when the frame
    // is drawn, so a direction change mirrors it.
    const bool flip = d->reverse != SEGMENT.reverse;
    for (unsigned i = 0; i < SEGLEN; i++) startFrame[i] = SEGMENT.getPixelColor(flip ? SEGLEN - 1 - i : i);
  }
  if (fresh || retarget) {
    unsigned first = 0;
    while (first < SEGLEN && startFrame[first] == color) first++; // LEDs already at the target need no sweep
    d->startPx = first;
    d->color   = color;
    d->color2  = color2;
    d->reverse = SEGMENT.reverse;
    d->trigger = sweepTrigger;
    SEGENV.step = millis(); // real time, not strip.now: a timebase sync must not jump the sweep
  }
  d->segOn = segOn;

  unsigned gradWidth = map(SEGMENT.intensity, 0, 255, 1, SEGLEN / 2);
  if (gradWidth < 1) gradWidth = 1;

  // Positions in 1/256 pixel so the front glides instead of jumping a whole LED per step.
  // The sweep runs past the strip end so the gradient fully clears the last LED.
  // Speed sets the duration of a full-length sweep; a sweep starting further in is shorter.
  const uint32_t cycleTime = 750 + (255 - SEGMENT.speed) * 150;
  const uint32_t grad      = gradWidth << 8;
  const uint32_t sweepEnd  = (SEGLEN - 1 + gradWidth) << 8;
  const uint32_t elapsed   = millis() - SEGENV.step;
  const uint64_t front     = ((uint64_t)d->startPx << 8) + (uint64_t)elapsed * sweepEnd / cycleTime;
  if (front >= sweepEnd) {
    // sweep finished: keep repainting so color changes still show up
    SEGMENT.fill(color);
    return;
  }

  for (unsigned i = 0; i < SEGLEN; i++) {
    const uint32_t pos = i << 8;
    if (pos > front) {
      SEGMENT.setPixelColor(i, startFrame[i]);
      continue;
    }
    const uint32_t distanceFromFront = front - pos;
    if (distanceFromFront >= grad) {
      SEGMENT.setPixelColor(i, color);
    } else {
      // gradient zone: start color at the leading edge, target color deeper in
      uint8_t fadeAmount = (255 * distanceFromFront) / grad;
      if (fadeAmount < 20) fadeAmount = 20; // avoid low-PWM color distortion on LEDs
      SEGMENT.setPixelColor(i, color_blend(startFrame[i], color, fadeAmount));
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
