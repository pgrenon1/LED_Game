#include <FastLED.h>

// --- Configuration ---
#define LED_PIN     9
#define NUM_LEDS    90
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define BTN_LEFT    2
#define BTN_RIGHT   3

#define BRIGHTNESS  255
#define BACKGROUND_BRIGHTNESS 2

#define INITIAL_TARGET_ZONE_BREATHING_RATE 28
#define TARGET_ZONE_MIN_BRIGHTNESS 60
#define LEFT_ENDPOINT_GLOW_RATE 30
#define LEFT_ENDPOINT_MIN_BRIGHTNESS 100
#define RIGHT_ENDPOINT_GLOW_RATE 32
#define RIGHT_ENDPOINT_MIN_BRIGHTNESS 100

#define INITIAL_TARGET_ZONE_SIZE 8
#define MIN_TARGET_ZONE_SIZE 1
#define TARGET_ZONE_EDGE_MARGIN 10
#define TARGET_ZONE_BREATHING_RATE_STEP 16
#define MAX_MISS_STREAK 3
#define PULSE_MOVE_INTERVAL_MS 15
#define MISS_MARKER_DURATION_FRAMES 7

// --- Global Variables ---
CRGB leds[NUM_LEDS];

int leftPulsePosition = -1;
int rightPulsePosition = -1;
bool leftPulseActive = false;
bool rightPulseActive = false;

int targetZoneStart = 0;
int targetZoneEnd = 0;
int currentTargetZoneSize = INITIAL_TARGET_ZONE_SIZE;
int currentTargetZoneBreathingRate = INITIAL_TARGET_ZONE_BREATHING_RATE;

int missStreak = 0;

unsigned long lastPulseMoveTime = 0;

bool missMarkerActive = false;
int missMarkerCenter = 0;
int missMarkerFrame = 0;

// --- Function Prototypes ---
void relocateTargetZone();
void removePulses();
void handleMiss(int collisionSpanCenter);
void handleHit();
void handleSerial();
void handleInput();
void movePulses();
void fadeBackground();
void checkCollision();
void drawScene();
void updateMissMarker();
void runHitAnimation();
void runProgressResetAnimation();

// --- Main Program ---

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  randomSeed(analogRead(A0));
  relocateTargetZone();
}

void loop() {
  handleSerial();
  handleInput();
  movePulses();
  fadeBackground();
  drawScene();
  updateMissMarker();
  FastLED.show();
}

// --- Logic Functions ---

void relocateTargetZone() {
  targetZoneStart = random(
      TARGET_ZONE_EDGE_MARGIN,
      NUM_LEDS - currentTargetZoneSize - TARGET_ZONE_EDGE_MARGIN + 1);
  targetZoneEnd = targetZoneStart + currentTargetZoneSize - 1;
}

void removePulses() {
  leftPulseActive = false;
  rightPulseActive = false;
  leftPulsePosition = -1;
  rightPulsePosition = -1;
}

void handleSerial() {
  static String inputBuffer = "";
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer == "miss") {
        Serial.println("ACK: miss");
        handleMiss(NUM_LEDS / 2);
      }
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }
}

void handleInput() {
  if (!leftPulseActive && digitalRead(BTN_LEFT) == LOW) {
    leftPulseActive = true;
    leftPulsePosition = 0;
  }
  if (!rightPulseActive && digitalRead(BTN_RIGHT) == LOW) {
    rightPulseActive = true;
    rightPulsePosition = NUM_LEDS - 1;
  }
}

void movePulses() {
  if (millis() - lastPulseMoveTime < PULSE_MOVE_INTERVAL_MS) return;
  lastPulseMoveTime = millis();

  if (leftPulseActive) leftPulsePosition++;
  if (rightPulseActive) rightPulsePosition--;

  if (leftPulseActive && rightPulseActive && leftPulsePosition >= rightPulsePosition) {
    checkCollision();
  }

  if (leftPulsePosition >= NUM_LEDS) leftPulseActive = false;
  if (rightPulsePosition < 0) rightPulseActive = false;
}

void fadeBackground() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(10);
    if (leds[i].r < BACKGROUND_BRIGHTNESS) leds[i].r = BACKGROUND_BRIGHTNESS;
    if (leds[i].g < BACKGROUND_BRIGHTNESS) leds[i].g = BACKGROUND_BRIGHTNESS;
    if (leds[i].b < BACKGROUND_BRIGHTNESS) leds[i].b = BACKGROUND_BRIGHTNESS;
  }
}

void handleMiss(int collisionSpanCenter) {
  missStreak++;

  if (missStreak < MAX_MISS_STREAK) {
    Serial.print("Miss Streak: ");
    Serial.print(missStreak);
    Serial.print("/");
    Serial.println(MAX_MISS_STREAK);

    missMarkerActive = true;
    missMarkerCenter = collisionSpanCenter;
    missMarkerFrame = 0;
  } else {
    Serial.println("Progress Reset.");
    runProgressResetAnimation();
    currentTargetZoneSize = INITIAL_TARGET_ZONE_SIZE;
    currentTargetZoneBreathingRate = INITIAL_TARGET_ZONE_BREATHING_RATE;
    missStreak = 0;
    relocateTargetZone();
  }

  removePulses();
}

void handleHit() {
  missStreak = 0;
  runHitAnimation();

  if (currentTargetZoneSize > MIN_TARGET_ZONE_SIZE) {
    currentTargetZoneSize--;
    currentTargetZoneBreathingRate += TARGET_ZONE_BREATHING_RATE_STEP;
  }

  relocateTargetZone();
  removePulses();
}

void checkCollision() {
  int collisionSpanStart =
      leftPulsePosition < rightPulsePosition ? leftPulsePosition : rightPulsePosition;
  int collisionSpanEnd =
      leftPulsePosition > rightPulsePosition ? leftPulsePosition : rightPulsePosition;

  if (collisionSpanEnd >= targetZoneStart && collisionSpanStart <= targetZoneEnd) {
    handleHit();
  } else {
    handleMiss((collisionSpanStart + collisionSpanEnd) / 2);
  }
}

void drawScene() {
  uint8_t targetZoneBrightness =
      beatsin8(currentTargetZoneBreathingRate, TARGET_ZONE_MIN_BRIGHTNESS, 255);
  for (int i = targetZoneStart; i <= targetZoneEnd; i++) {
    leds[i] = CHSV(40, 255, targetZoneBrightness);
  }

  uint8_t leftEndpointBrightness =
      beatsin8(LEFT_ENDPOINT_GLOW_RATE, LEFT_ENDPOINT_MIN_BRIGHTNESS, 255);
  uint8_t rightEndpointBrightness =
      beatsin8(RIGHT_ENDPOINT_GLOW_RATE, RIGHT_ENDPOINT_MIN_BRIGHTNESS, 255);

  if (!leftPulseActive) {
    leds[0] = CRGB(leftEndpointBrightness, leftEndpointBrightness, leftEndpointBrightness);
  }
  if (!rightPulseActive) {
    leds[NUM_LEDS - 1] =
        CRGB(rightEndpointBrightness, rightEndpointBrightness, rightEndpointBrightness);
  }

  if (leftPulseActive && leftPulsePosition >= 0 && leftPulsePosition < NUM_LEDS) {
    leds[leftPulsePosition] = CRGB::White;
  }
  if (rightPulseActive && rightPulsePosition >= 0 && rightPulsePosition < NUM_LEDS) {
    leds[rightPulsePosition] = CRGB::White;
  }
}

void updateMissMarker() {
  if (!missMarkerActive) return;
  for (int offset = -1; offset <= 1; offset++) {
    int position = missMarkerCenter + offset;
    if (position >= 0 && position < NUM_LEDS) leds[position] = CRGB::Red;
  }
  missMarkerFrame++;
  if (missMarkerFrame >= MISS_MARKER_DURATION_FRAMES) missMarkerActive = false;
}

void runHitAnimation() {
  const int frameDelayMs = 20;
  const int midBrightness = 64;
  for (int i = 0; i <= 10; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, (uint8_t)(255.0 * i / 10.0), 0));
    FastLED.show(); delay(frameDelayMs);
  }
  for (int i = 0; i <= 10; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 255 - (uint8_t)((255 - midBrightness) * i / 10.0), 0));
    FastLED.show(); delay(frameDelayMs);
  }
  for (int i = 0; i <= 15; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, midBrightness + (uint8_t)((255 - midBrightness) * i / 15.0), 0));
    FastLED.show(); delay(frameDelayMs);
  }
  for (int i = 0; i <= 30; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 255 - (uint8_t)(255 * i / 30.0), 0));
    FastLED.show(); delay(frameDelayMs);
  }
}

void runProgressResetAnimation() {
  for (int i = 0; i < 16; i++) {
    uint8_t brightness = (i % 4 < 2) ? 255 : 50;
    fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, 0));
    FastLED.show();
    delay(40);
  }
}
