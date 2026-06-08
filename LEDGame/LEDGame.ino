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

#define INITIAL_GOAL_PULSE_SPEED 28
#define GOAL_PULSE_MIN 60
#define EDGE_LEFT_PULSE_SPEED 30
#define EDGE_LEFT_PULSE_MIN 100
#define EDGE_RIGHT_PULSE_SPEED 32
#define EDGE_RIGHT_PULSE_MIN 100

#define INITIAL_GOAL_SIZE 8
#define MAX_FAILS   3

// --- Global Variables ---
CRGB leds[NUM_LEDS];

int leftPos = -1;
int rightPos = -1;
bool leftActive = false;
bool rightActive = false;

int goalStart = 0;
int goalEnd = 0;
int currentGoalSize = INITIAL_GOAL_SIZE;
int currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;

int failCount = 0;
int score = 0;

unsigned long lastMoveTime = 0;
int pulseSpeed = 15;

bool redExplosionActive = false;
int redExplosionCenter = 0;
int redExplosionFrame = 0;
const int redExplosionSteps = 7;

// --- Function Prototypes ---
void generateLevel();
void resetPulses();
void playerWins();
void playerFails(int explosionCenter);
void displayScoreAnimation();
void handleSerial();
void handleInput();
void movePulses();
void fadeBackground();
void checkCollision();
void drawScene();
void updateRedExplosion();
void winAnimation2();
void loseAnimation();

// --- Main Program ---

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  randomSeed(analogRead(A0));
  currentGoalSize = INITIAL_GOAL_SIZE;
  currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;
  generateLevel(); // sets initial goal
}

void loop() {
  handleSerial();
  handleInput();
  movePulses();
  fadeBackground();
  updateRedExplosion();
  drawScene();
  FastLED.show();
}

// --- Logic Functions ---

void generateLevel() {
  goalStart = random(10, NUM_LEDS - currentGoalSize - 10);
  goalEnd = goalStart + currentGoalSize;
  failCount = 0;
}

void resetPulses() {
  leftActive = false;
  rightActive = false;
  leftPos = -1;
  rightPos = -1;
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
  if (!leftActive && digitalRead(BTN_LEFT) == LOW) {
    leftActive = true;
    leftPos = 0;
  }
  if (!rightActive && digitalRead(BTN_RIGHT) == LOW) {
    rightActive = true;
    rightPos = NUM_LEDS - 1;
  }
}

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
  
  winAnimation2();          // Green Pulse
  displayScoreAnimation();  // 2s Blue Score Display
  
  generateLevel();
  resetPulses();
}

void displayScoreAnimation() {
  FastLED.clear();
  for (int i = 0; i < score && i < (NUM_LEDS / 2); i++) {
    leds[i] = CRGB::Blue;
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
    redExplosionFrame = 0;
  } else {
    Serial.println("GAME OVER! Resetting game.");
    score = 0;
    failCount = 0;
    loseAnimation();
    generateLevel();
  }
  resetPulses();
}

void checkCollision() {
  if (leftPos >= goalStart && leftPos <= goalEnd) {
    failCount = 0;
    winAnimation2();
    
    // Decrease goal size for next round, min size 1
    if (currentGoalSize > 1) {
      currentGoalSize--;
      currentGoalPulseSpeed += 16; // Speed up the breathing animation more significantly
    }
    
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
      currentGoalSize = INITIAL_GOAL_SIZE; // Reset on game over
      currentGoalPulseSpeed = INITIAL_GOAL_PULSE_SPEED;
      generateLevel();
    }

    resetPulses();
  }
}

void drawScene() {
  uint8_t glow = beatsin8(currentGoalPulseSpeed, GOAL_PULSE_MIN, 255);
  for (int i = goalStart; i <= goalEnd; i++) {
    leds[i] = CHSV(40, 255, glow);
  }

  uint8_t edgePulseLeft = beatsin8(EDGE_LEFT_PULSE_SPEED, EDGE_LEFT_PULSE_MIN, 255);
  uint8_t edgePulseRight = beatsin8(EDGE_RIGHT_PULSE_SPEED, EDGE_RIGHT_PULSE_MIN, 255);

  if (!leftActive) leds[0] = CRGB(edgePulseLeft, edgePulseLeft, edgePulseLeft);
  if (!rightActive) leds[NUM_LEDS - 1] = CRGB(edgePulseRight, edgePulseRight, edgePulseRight);

  if (leftActive && leftPos >= 0 && leftPos < NUM_LEDS) leds[leftPos] = CRGB::White;
  if (rightActive && rightPos >= 0 && rightPos < NUM_LEDS) leds[rightPos] = CRGB::White;
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
