#include <FastLED.h>

// --- Configuration ---
#define LED_PIN     9
#define NUM_LEDS    90
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define BTN_LEFT    2
#define BTN_RIGHT   3

#define BRIGHTNESS              255
#define BACKGROUND_BRIGHTNESS   2

#define INITIAL_GOAL_PULSE_SPEED  28
#define GOAL_PULSE_MIN            60
#define EDGE_LEFT_PULSE_SPEED     30
#define EDGE_LEFT_PULSE_MIN       100
#define EDGE_RIGHT_PULSE_SPEED    32
#define EDGE_RIGHT_PULSE_MIN      100

#define INITIAL_GOAL_SIZE   8
#define MAX_FAILS           3

// Grow/retract speed (ms per LED step)
#define GROW_SPEED    18
#define RETRACT_SPEED 12

// --- Global Variables ---
CRGB leds[NUM_LEDS];

// leftTip: rightmost lit LED of the left line (grows from 0 rightward)
// rightTip: leftmost lit LED of the right line (grows from NUM_LEDS-1 leftward)
int leftTip  = -1;   // -1 = line fully retracted (not visible)
int rightTip = NUM_LEDS; // NUM_LEDS = fully retracted

bool leftHeld  = false;
bool rightHeld = false;

// Must release button after a round before it can grow again
bool leftReady  = false;
bool rightReady = false;

int goalStart = 0;
int goalEnd   = 0;
int currentGoalSize       = INITIAL_GOAL_SIZE;
int currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;

int failCount = 0;
int score     = 0;

unsigned long lastMoveTime = 0;

bool redExplosionActive = false;
int  redExplosionCenter = 0;
int  redExplosionFrame  = 0;
const int redExplosionSteps = 7;

bool collisionHandled = false; // prevents repeated collision triggers

// --- Function Prototypes ---
void generateLevel();
void resetLines();
void playerWins();
void playerFails(int explosionCenter);
void displayScoreAnimation();
void handleSerial();
void handleInput();
void moveLines();
void fadeBackground();
void checkCollision();
void drawScene();
void updateRedExplosion();
void winAnimation2();
void loseAnimation();

// --- Setup & Loop ---

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  randomSeed(analogRead(A0));
  currentGoalSize       = INITIAL_GOAL_SIZE;
  currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;
  generateLevel();
}

void loop() {
  handleSerial();
  handleInput();
  moveLines();
  fadeBackground();
  updateRedExplosion();
  drawScene();
  FastLED.show();
}

// --- Logic Functions ---

void generateLevel() {
  goalStart = random(10, NUM_LEDS - currentGoalSize - 10);
  goalEnd   = goalStart + currentGoalSize;
  failCount = 0;
}

void resetLines() {
  leftTip          = -1;
  rightTip         = NUM_LEDS;
  leftHeld         = false;
  rightHeld        = false;
  leftReady        = false;  // must release before growing again
  rightReady       = false;
  collisionHandled = false;
}

void handleSerial() {
  static String inputBuffer = "";
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer == "score") {
        Serial.println("ACK: score");
        playerWins();
      } else if (inputBuffer == "fail") {
        Serial.println("ACK: fail");
        playerFails(NUM_LEDS / 2);
      }
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }
}

void handleInput() {
  bool btnLeft  = (digitalRead(BTN_LEFT)  == LOW);
  bool btnRight = (digitalRead(BTN_RIGHT) == LOW);

  // Become ready only once the button has been released after a reset
  if (!btnLeft)  leftReady  = true;
  if (!btnRight) rightReady = true;

  leftHeld  = btnLeft  && leftReady;
  rightHeld = btnRight && rightReady;
}

void moveLines() {
  if (millis() - lastMoveTime < GROW_SPEED) return;
  lastMoveTime = millis();

  // Left line: grows right when held, retracts left when released
  if (leftHeld) {
    if (leftTip < NUM_LEDS - 1) leftTip++;
  } else {
    if (leftTip >= 0) leftTip--;
  }

  // Right line: grows left when held, retracts right when released
  if (rightHeld) {
    if (rightTip > 0) rightTip--;
  } else {
    if (rightTip < NUM_LEDS) rightTip++;
  }

  // Check if tips have met or crossed
  if (leftTip >= 0 && rightTip < NUM_LEDS && leftTip >= rightTip) {
    if (!collisionHandled) {
      collisionHandled = true;
      checkCollision();
    }
  } else {
    collisionHandled = false;
  }
}

void fadeBackground() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(10);
    if (leds[i].r < BACKGROUND_BRIGHTNESS) leds[i].r = BACKGROUND_BRIGHTNESS;
    if (leds[i].g < BACKGROUND_BRIGHTNESS) leds[i].g = BACKGROUND_BRIGHTNESS;
    if (leds[i].b < BACKGROUND_BRIGHTNESS) leds[i].b = BACKGROUND_BRIGHTNESS;
  }
}

void playerWins() {
  score++;
  Serial.print("Point scored! Total Score: ");
  Serial.println(score);

  winAnimation2();

  if (currentGoalSize > 1) {
    currentGoalSize--;
    currentGoalPulseSpeed += 16;
  }

  generateLevel();
  resetLines();
}

void displayScoreAnimation() {
  FastLED.clear();
  for (int i = 0; i < score && i < (NUM_LEDS / 2); i++) {
    leds[i]                = CRGB::Blue;
    leds[NUM_LEDS - 1 - i] = CRGB::Blue;
  }
  FastLED.show();
  delay(2000);
}

void playerFails(int explosionCenter) {
  failCount++;

  if (failCount < MAX_FAILS) {
    Serial.print("Missed! Deaths: ");
    Serial.print(failCount);
    Serial.print("/");
    Serial.println(MAX_FAILS);

    redExplosionActive = true;
    redExplosionCenter = explosionCenter;
    redExplosionFrame  = 0;
  } else {
    Serial.println("GAME OVER! Resetting game.");
    score     = 0;
    failCount = 0;
    loseAnimation();
    currentGoalSize       = INITIAL_GOAL_SIZE;
    currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;
    generateLevel();
  }
  resetLines();
}

void checkCollision() {
  // Meeting point is roughly where the tips crossed
  int meetPoint = (leftTip + rightTip) / 2;

  // Win: meeting point is inside the goal zone
  if (meetPoint >= goalStart && meetPoint <= goalEnd) {
    playerWins();
  } else {
    // Fail: determine explosion center and trigger fail
    redExplosionActive = true;
    redExplosionCenter = meetPoint;
    redExplosionFrame  = 0;

    failCount++;
    if (failCount < MAX_FAILS) {
      Serial.print("Missed! Deaths: ");
      Serial.print(failCount);
      Serial.print("/");
      Serial.println(MAX_FAILS);
      resetLines();
    } else {
      Serial.println("GAME OVER! Resetting game.");
      score     = 0;
      failCount = 0;
      loseAnimation();
      currentGoalSize       = INITIAL_GOAL_SIZE;
      currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;
      generateLevel();
      resetLines();
    }
  }
}

void drawScene() {
  // Draw goal zone (pulsing amber/yellow)
  uint8_t glow = beatsin8(currentGoalPulseSpeed, GOAL_PULSE_MIN, 255);
  for (int i = goalStart; i <= goalEnd; i++) {
    leds[i] = CHSV(40, 255, glow);
  }

  // Edge indicators when lines are fully retracted
  uint8_t edgePulseLeft  = beatsin8(EDGE_LEFT_PULSE_SPEED,  EDGE_LEFT_PULSE_MIN,  255);
  uint8_t edgePulseRight = beatsin8(EDGE_RIGHT_PULSE_SPEED, EDGE_RIGHT_PULSE_MIN, 255);

  if (leftTip < 0)          leds[0]            = CRGB(edgePulseLeft,  edgePulseLeft,  edgePulseLeft);
  if (rightTip >= NUM_LEDS) leds[NUM_LEDS - 1] = CRGB(edgePulseRight, edgePulseRight, edgePulseRight);

  // Draw left line: LEDs 0 .. leftTip (cyan-ish blue/white)
  if (leftTip >= 0) {
    for (int i = 0; i <= leftTip && i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 180, 255); // cyan
    }
    // Bright white tip
    leds[leftTip] = CRGB::White;
  }

  // Draw right line: LEDs rightTip .. NUM_LEDS-1 (magenta-ish)
  if (rightTip < NUM_LEDS) {
    for (int i = rightTip; i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 0, 180); // magenta
    }
    // Bright white tip
    leds[rightTip] = CRGB::White;
  }
}

void updateRedExplosion() {
  if (!redExplosionActive) return;
  for (int i = -1; i <= 1; i++) {
    int pos = redExplosionCenter + i;
    if (pos >= 0 && pos < NUM_LEDS) leds[pos] = CRGB::Red;
  }
  redExplosionFrame++;
  if (redExplosionFrame >= redExplosionSteps) redExplosionActive = false;
}

void winAnimation2() {
  const int delayTime = 20;
  const int midBrightness = 64;
  for (int i = 0; i <= 10; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, (uint8_t)(255.0 * i / 10.0), 0));
    FastLED.show(); delay(delayTime);
  }
  for (int i = 0; i <= 10; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 255 - (uint8_t)((255 - midBrightness) * i / 10.0), 0));
    FastLED.show(); delay(delayTime);
  }
  for (int i = 0; i <= 15; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, midBrightness + (uint8_t)((255 - midBrightness) * i / 15.0), 0));
    FastLED.show(); delay(delayTime);
  }
  for (int i = 0; i <= 30; i++) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 255 - (uint8_t)(255 * i / 30.0), 0));
    FastLED.show(); delay(delayTime);
  }
}

void loseAnimation() {
  for (int i = 0; i < 16; i++) {
    uint8_t brightness = (i % 4 < 2) ? 255 : 50;
    fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, 0));
    FastLED.show();
    delay(40);
  }
}
