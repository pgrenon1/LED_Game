#include <FastLED.h>

#define LED_PIN     9
#define NUM_LEDS    90
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define BTN_LEFT    2
#define BTN_RIGHT   3

#define BRIGHTNESS  255
#define BACKGROUND_BRIGHTNESS 2

#define GOAL_PULSE_SPEED 28
#define GOAL_PULSE_MIN 100
#define EDGE_LEFT_PULSE_SPEED 30
#define EDGE_LEFT_PULSE_MIN 100
#define EDGE_RIGHT_PULSE_SPEED 32
#define EDGE_RIGHT_PULSE_MIN 100

#define GOAL_SIZE   8
#define MAX_FAILS   3

CRGB leds[NUM_LEDS];

// Pulse state
int leftPos = -1;
int rightPos = -1;
bool leftActive = false;
bool rightActive = false;

// Goal (static, only changes on 3 fails)
int goalStart = 0;
int goalEnd = 0;

// Game state
int failCount = 0;

unsigned long lastMoveTime = 0;
int pulseSpeed = 15;

// Red explosion (non-blocking)
bool redExplosionActive = false;
int redExplosionCenter = 0;
int redExplosionFrame = 0;
const int redExplosionSteps = 7;  // Longer duration
const int redExplosionSpread = 3;  // fading spread beyond the 3 pixels

// --------------------------------------------------

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  randomSeed(analogRead(A0));
  generateLevel(); // sets initial goal
}

// --------------------------------------------------

void loop() {
  handleInput();
  movePulses();
  fadeBackground();
  updateRedExplosion();
  drawScene();

  FastLED.show();
}

// --------------------------------------------------

void generateLevel() {
  goalStart = random(10, NUM_LEDS - GOAL_SIZE - 10);
  goalEnd = goalStart + GOAL_SIZE;
  failCount = 0;
}

// --------------------------------------------------

void resetPulses() {
  leftActive = false;
  rightActive = false;
  leftPos = -1;
  rightPos = -1;
}

// --------------------------------------------------

void handleInput() {
  if (!leftActive && digitalRead(BTN_LEFT) == LOW) {
    leftActive = true;
    leftPos = 0;
  }

  if (!rightActive && digitalRead(BTN_RIGHT) == LOW) {
    rightActive = true;
    rightPos = NUM_LEDS - 1;
  }
}

// --------------------------------------------------

void movePulses() {
  if (millis() - lastMoveTime < pulseSpeed) return;
  lastMoveTime = millis();

  if (leftActive) leftPos++;
  if (rightActive) rightPos--;

  if (leftActive && rightActive && leftPos >= rightPos) {
    checkCollision();
  }

  if (leftPos >= NUM_LEDS) leftActive = false;
  if (rightPos < 0) rightActive = false;
}

// --------------------------------------------------

void fadeBackground() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(10);
    // Faint pink background tint
    if (leds[i].r < BACKGROUND_BRIGHTNESS) leds[i].r = BACKGROUND_BRIGHTNESS;
    if (leds[i].g < 0) leds[i].g = 0; 
    if (leds[i].b < BACKGROUND_BRIGHTNESS / 2) leds[i].b = BACKGROUND_BRIGHTNESS / 2;
  }
}

// --------------------------------------------------

void checkCollision() {
  if (leftPos >= goalStart && leftPos <= goalEnd) {
    failCount = 0;
    winAnimation2();
    generateLevel();   // NEW: create a new goal after winning
    resetPulses();
  } else {
    failCount++;
    
    if (failCount < MAX_FAILS) {
      // Tiny red explosion (3 pixels wide, non-blocking)
      redExplosionActive = true;
      redExplosionCenter = (leftPos + rightPos) / 2;
      redExplosionFrame = 0;
    } else {
      // Long red lose animation + goal change
      loseAnimation();
      generateLevel();
    }

    resetPulses();
  }
}

// --------------------------------------------------

void drawScene() {
  uint8_t glow = beatsin8(GOAL_PULSE_SPEED, GOAL_PULSE_MIN, 255);
  for (int i = goalStart; i <= goalEnd; i++) {
    leds[i] = CHSV(235, 200, glow); // Cupid Pink Goal
  }

  uint8_t edgePulseLeft = beatsin8(EDGE_LEFT_PULSE_SPEED, EDGE_LEFT_PULSE_MIN, 255);
  uint8_t edgePulseRight = beatsin8(EDGE_RIGHT_PULSE_SPEED, EDGE_RIGHT_PULSE_MIN, 255);

  if (!leftActive)
    leds[0] = CRGB(edgePulseLeft, 0, edgePulseLeft / 2); // Soft Rose Edge

  if (!rightActive)
    leds[NUM_LEDS - 1] = CRGB(edgePulseRight, 0, edgePulseRightpi / 2); // Soft Rose Edge

  if (leftActive && leftPos >= 0 && leftPos < NUM_LEDS)
    leds[leftPos] = CRGB::White; // "Arrow" Pulse
  if (rightActive && rightPos >= 0 && rightPos < NUM_LEDS)
    leds[rightPos] = CRGB::White; // "Arrow" Pulse
}

// --------------------------------------------------

// Non-blocking tiny red explosion (3 pixels wide + fading spread)
void updateRedExplosion() {
  if (!redExplosionActive) return;

  int t = redExplosionFrame;
  int steps = redExplosionSteps;
  int spread = redExplosionSpread;

  // Three central pixels
  for (int i = -1; i <= 1; i++) {
    int pos = redExplosionCenter + i;
    if (pos >= 0 && pos < NUM_LEDS) {
      leds[pos] = CRGB::Red;
    }
  }

  redExplosionFrame++;
  if (redExplosionFrame >= redExplosionSteps) {
    redExplosionActive = false;
  }
}

// --------------------------------------------------

// Smooth pulsing win animation (fully green, gentle)
void winAnimation1() {
  const int frames = 50;       // frames for the pulse
  const int delayTime = 20;     // ms per frame
  for (int i = 0; i <= frames; i++) {
    // Smooth single pulse: 0 → 255 → 0 over the frames
    float phase = (float)i / frames;                   // 0 → 1
    uint8_t brightness = (uint8_t)(255.0 * sin(phase * 3.14159)); // smooth bump

    // Apply to entire strip
    fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, brightness / 2));
    FastLED.show();
    delay(delayTime);
  }
}

void winAnimation2() {
  const int frames1 = 10;  // frames for first rise 0 → 255
  const int frames2 = 10;  // frames for dip 255 → 196
  const int frames3 = 15;  // frames for rise 196 → 255
  const int frames4 = 30;  // frames for final fade 255 → 0
  const int delayTime = 20; // ms per frame
  
  const int midBrightness = 64; 

  // 0 → 255
  for (int i = 0; i <= frames1; i++) {
    float phase = (float)i / frames1;
    uint8_t b = (uint8_t)(255.0 * phase); // linear rise
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, b / 2));
    FastLED.show();
    delay(delayTime);
  }

  // 255 → 196
  for (int i = 0; i <= frames2; i++) {
    float phase = (float)i / frames2;
    uint8_t b = 255 - (uint8_t)((255 - midBrightness) * phase);
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, b / 2));
    FastLED.show();
    delay(delayTime);
  }

  // 196 → 255
  for (int i = 0; i <= frames3; i++) {
    float phase = (float)i / frames3;
    uint8_t b = midBrightness + (uint8_t)((255 - midBrightness) * phase);
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, b / 2));
    FastLED.show();
    delay(delayTime);
  }

  // 255 → 0
  for (int i = 0; i <= frames4; i++) {
    float phase = (float)i / frames4;
    uint8_t b = 255 - (uint8_t)(255 * phase);
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, b / 2));
    FastLED.show();
    delay(delayTime);
  }
}

// Smooth pulsing lose animation (fully red, slower hard pulse)
void loseAnimation() {
  for (int i = 0; i < 16; i++) {
    uint8_t b = (i % 4 < 2) ? 255 : 50; // slower flicker
    CRGB color = (i % 2 == 0) ? CRGB(b, 0, 0) : CRGB(b / 2, 0, b); // Red and Purple
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
    delay(40);
  }
}
