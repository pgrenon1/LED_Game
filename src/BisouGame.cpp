#include <Arduino.h>
#include "BisouGame.h"
#include "StripEngine.h"

void BisouGame::onMounted()
{
  Serial.begin(9600);
  currentTargetZoneSize_ = kInitialTargetZoneSize;
  currentTargetZoneBreathingRate_ = kInitialTargetZoneBreathingRate;
  missStreak_ = 0;
  removePulses();
  relocateTargetZone();
}

void BisouGame::update()
{
  handleSerial();
  handleInput();
  movePulses();
  fadeBackground();
}

void BisouGame::render()
{
  drawScene();
  updateMissMarker();
}

void BisouGame::relocateTargetZone()
{
  targetZoneStart_ = random(kTargetZoneEdgeMargin, engine().ledCount() - currentTargetZoneSize_ - kTargetZoneEdgeMargin + 1);
  targetZoneEnd_ = targetZoneStart_ + currentTargetZoneSize_ - 1;
}

void BisouGame::removePulses()
{
  leftPulseActive_ = rightPulseActive_ = false;
  leftPulsePosition_ = rightPulsePosition_ = -1;
}

void BisouGame::handleSerial()
{
  while (Serial.available())
  {
    const char c = Serial.read();
    if (c == '\n')
    {
      serialBuffer_.trim();
      if (serialBuffer_ == "miss")
      {
        Serial.println("ACK: miss");
        handleMiss(engine().ledCount() / 2);
      }
      serialBuffer_ = "";
    }
    else if (c != '\r')
      serialBuffer_ += c;
  }
}

void BisouGame::handleInput()
{
  if (!leftPulseActive_ && engine().buttonIsPressed(StripEngine::Button::Left))
  {
    leftPulseActive_ = true;
    leftPulsePosition_ = 0;
  }
  if (!rightPulseActive_ && engine().buttonIsPressed(StripEngine::Button::Right))
  {
    rightPulseActive_ = true;
    rightPulsePosition_ = engine().ledCount() - 1;
  }
}

void BisouGame::movePulses()
{
  if (millis() - lastPulseMoveTime_ < kPulseMoveIntervalMs)
    return;
  lastPulseMoveTime_ = millis();
  if (leftPulseActive_)
    ++leftPulsePosition_;
  if (rightPulseActive_)
    --rightPulsePosition_;
  if (leftPulseActive_ && rightPulseActive_ && leftPulsePosition_ >= rightPulsePosition_)
    checkCollision();
  if (leftPulsePosition_ >= static_cast<int>(engine().ledCount()))
    leftPulseActive_ = false;
  if (rightPulsePosition_ < 0)
    rightPulseActive_ = false;
}

void BisouGame::fadeBackground()
{
  CRGB *leds = engine().leds();
  for (uint16_t i = 0; i < engine().ledCount(); ++i)
  {
    leds[i].fadeToBlackBy(10);
    if (leds[i].r < kBackgroundBrightness)
      leds[i].r = kBackgroundBrightness;
    if (leds[i].g < kBackgroundBrightness)
      leds[i].g = kBackgroundBrightness;
    if (leds[i].b < kBackgroundBrightness)
      leds[i].b = kBackgroundBrightness;
  }
}

void BisouGame::checkCollision()
{
  const int start = min(leftPulsePosition_, rightPulsePosition_), end = max(leftPulsePosition_, rightPulsePosition_);
  if (end >= targetZoneStart_ && start <= targetZoneEnd_)
    handleHit();
  else
    handleMiss((start + end) / 2);
}

void BisouGame::handleMiss(int center)
{
  if (++missStreak_ < kMaxMissStreak)
  {
    Serial.print("Miss Streak: ");
    Serial.print(missStreak_);
    Serial.print("/");
    Serial.println(kMaxMissStreak);
    missMarkerActive_ = true;
    missMarkerCenter_ = center;
    missMarkerFrame_ = 0;
  }
  else
  {
    Serial.println("Progress Reset.");
    runProgressResetAnimation();
    currentTargetZoneSize_ = kInitialTargetZoneSize;
    currentTargetZoneBreathingRate_ = kInitialTargetZoneBreathingRate;
    missStreak_ = 0;
    relocateTargetZone();
  }
  removePulses();
}

void BisouGame::handleHit()
{
  missStreak_ = 0;
  runHitAnimation();
  if (currentTargetZoneSize_ > kMinTargetZoneSize)
  {
    --currentTargetZoneSize_;
    currentTargetZoneBreathingRate_ += kTargetZoneBreathingRateStep;
  }
  relocateTargetZone();
  removePulses();
}

void BisouGame::drawScene()
{
  CRGB *leds = engine().leds();
  const uint8_t zone = beatsin8(currentTargetZoneBreathingRate_, kTargetZoneMinBrightness, 255);
  for (int i = targetZoneStart_; i <= targetZoneEnd_; ++i)
    leds[i] = CHSV(40, 255, zone);
  if (!leftPulseActive_)
    leds[0] = CRGB(beatsin8(kLeftEndpointGlowRate, kLeftEndpointMinBrightness, 255));
  if (!rightPulseActive_)
    leds[engine().ledCount() - 1] = CRGB(beatsin8(kRightEndpointGlowRate, kRightEndpointMinBrightness, 255));
  if (leftPulseActive_ && leftPulsePosition_ >= 0 && leftPulsePosition_ < static_cast<int>(engine().ledCount()))
    leds[leftPulsePosition_] = CRGB::White;
  if (rightPulseActive_ && rightPulsePosition_ >= 0 && rightPulsePosition_ < static_cast<int>(engine().ledCount()))
    leds[rightPulsePosition_] = CRGB::White;
}

void BisouGame::updateMissMarker()
{
  if (!missMarkerActive_)
    return;
  CRGB *leds = engine().leds();
  for (int o = -1; o <= 1; ++o)
  {
    const int p = missMarkerCenter_ + o;
    if (p >= 0 && p < static_cast<int>(engine().ledCount()))
      leds[p] = CRGB::Red;
  }
  if (++missMarkerFrame_ >= kMissMarkerDurationFrames)
    missMarkerActive_ = false;
}

void BisouGame::runHitAnimation()
{
  CRGB *leds = engine().leds();
  for (int i = 0; i <= 10; ++i)
  {
    fill_solid(leds, engine().ledCount(), CRGB(0, 255L * i / 10, 0));
    engine().show();
    delay(20);
  }
  for (int i = 0; i <= 10; ++i)
  {
    fill_solid(leds, engine().ledCount(), CRGB(0, 255 - (191L * i / 10), 0));
    engine().show();
    delay(20);
  }
  for (int i = 0; i <= 15; ++i)
  {
    fill_solid(leds, engine().ledCount(), CRGB(0, 64 + (191L * i / 15), 0));
    engine().show();
    delay(20);
  }
  for (int i = 0; i <= 30; ++i)
  {
    fill_solid(leds, engine().ledCount(), CRGB(0, 255 - (255L * i / 30), 0));
    engine().show();
    delay(20);
  }
}

void BisouGame::runProgressResetAnimation()
{
  for (int i = 0; i < 16; ++i)
  {
    const uint8_t b = i % 4 < 2 ? 255 : 50;
    fill_solid(engine().leds(), engine().ledCount(), CRGB(b, 0, 0));
    engine().show();
    delay(40);
  }
}
