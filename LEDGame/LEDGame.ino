#include <FastLED.h>

#define LED_PIN     9
#define NUM_LEDS    298
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define MAX_FAIL_COUNT 2

#define BTN_GREEN 2
#define BTN_BLUE  3

CRGB leds[NUM_LEDS];

int leftPos;
int rightPos;
bool leftActive = false;
bool rightActive = false;

int failCount; 
int targetCenter = 0;
int targetLengths[] = {16, 10, 8};

enum GameState { IDLE, PLAY, ANIM };
GameState state = IDLE;

bool successAnim = false;
uint8_t animStep = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started.");

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(8);

  pinMode(BTN_GREEN, INPUT_PULLUP);
  pinMode(BTN_BLUE,  INPUT_PULLUP);

  randomSeed(analogRead(A0));

  Serial.println("Starting first round.");
  newRound();
}

int getTargetLength(){
  return targetLengths[failCount];
}

int getTargetStart(){
  return targetCenter - getTargetLength() / 2;
}

int getTargetEnd(){
  return targetCenter + getTargetLength() / 2;
}

void newRound() {
  Serial.println("=== NEW ROUND ===");

  leftActive = rightActive = false;
  state = IDLE;

  int center = NUM_LEDS / 2;
  int offset = random(-120, 120);
  targetCenter = constrain(center + offset, 0, NUM_LEDS - 1);
  failCount = 0;

  Serial.print("Target Center: ");
  Serial.println(targetCenter);
  Serial.print("Target Start: ");
  Serial.println(getTargetStart());
  Serial.print("Target End: ");
  Serial.println(getTargetEnd());

  FastLED.clear();

  for (int i = getTargetStart(); i <= getTargetEnd(); i++) {
    leds[i] = CRGB::Yellow;
  }

  FastLED.show();
}

void retryRound(){
  Serial.println("=== RETRY ROUND ===");

  if (failCount >= MAX_FAIL_COUNT){
    Serial.println("Fail count exceeded. Starting new round.");
    newRound();
    return;
  }

  leftActive = rightActive = false;
  state = IDLE;

  failCount++;

  Serial.print("Fail Count: ");
  Serial.println(failCount);
  Serial.print("Target Start: ");
  Serial.println(getTargetStart());
  Serial.print("Target End: ");
  Serial.println(getTargetEnd());

  FastLED.clear();

  for (int i = getTargetStart(); i <= getTargetEnd(); i++) {
    leds[i] = CRGB::Yellow;
  }

  FastLED.show();
}

void loop() {
  static bool lastGreen = HIGH, lastBlue = HIGH;
  bool g = digitalRead(BTN_GREEN);
  bool b = digitalRead(BTN_BLUE);

  // Button detection
  if (state != ANIM) {
    if (lastGreen == HIGH && g == LOW && !leftActive) {
      Serial.println("Green button pressed → LEFT DOT LAUNCH");
      leftPos = 0;
      leftActive = true;
      if (state == IDLE) {
        state = PLAY;
        Serial.println("State → PLAY");
      }
    }

    if (lastBlue == HIGH && b == LOW && !rightActive) {
      Serial.println("Blue button pressed → RIGHT DOT LAUNCH");
      rightPos = NUM_LEDS - 1;
      rightActive = true;
      if (state == IDLE) {
        state = PLAY;
        Serial.println("State → PLAY");
      }
    }
  }

  lastGreen = g;
  lastBlue  = b;

  // Animation
  if (state == ANIM) {
    if (animStep == 0) {
      Serial.print("Animation started. Success = ");
      Serial.println(successAnim ? "YES" : "NO");
    }

    animStep++;
    if (animStep > 10) {
      Serial.println("Animation done, retrying round.");
      retryRound();
    }

    delay(40);
    return;
  }

  // GAME LOGIC
  if (state == PLAY) {

    if (leftActive) {
      leftPos++;
      if (leftPos >= NUM_LEDS) {
        leftActive = false;
        Serial.println("Left dot exited LED strip.");
      }
    }

    if (rightActive) {
      rightPos--;

      if (rightPos < 0) {
        rightActive = false;
        Serial.println("Right dot exited LED strip.");
      }
    }

    // Meeting / crossing detection
    if (leftActive && rightActive) {
      int meetPos = -1;

      if (leftPos == rightPos) {
        meetPos = leftPos;
        Serial.print("Exact collision at ");
        Serial.println(meetPos);
      } else if (leftPos + 1 == rightPos || rightPos + 1 == leftPos) {
        meetPos = (leftPos + rightPos) / 2;
        Serial.print("Crossing detected at ");
        Serial.println(meetPos);
      }

      if (meetPos != -1) {
        bool inTarget = (meetPos >= getTargetStart() && meetPos <= getTargetEnd());
        Serial.print("In target zone? ");
        Serial.println(inTarget ? "YES" : "NO");

        successAnim = inTarget;
        animStep = 0;
        state = ANIM;
        Serial.println("State → ANIM");
      }
    }

    // If both dots gone → fail
    if (state == PLAY && !leftActive && !rightActive) {
      Serial.println("Both dots expired without meeting → FAIL");
      successAnim = false;
      animStep = 0;
      state = ANIM;
    }
  }

  // DRAW FRAME
  for (int i = getTargetStart(); i <= getTargetEnd(); i++) {
    leds[i] = CRGB::Yellow;
  }

  if (leftActive && leftPos >= 0 && leftPos < NUM_LEDS)
    leds[leftPos] = CRGB::White;

  if (rightActive && rightPos >= 0 && rightPos < NUM_LEDS)
    leds[rightPos] = CRGB::White;

  FastLED.show();
}
