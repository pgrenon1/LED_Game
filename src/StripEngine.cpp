#include "StripEngine.h"

void StripEngine::begin()
{
  FastLED.addLeds<StripEngineConfig::LedChipset, StripEngineConfig::LedPin, StripEngineConfig::ColorOrder>(leds_, StripEngineConfig::LedCount);
  FastLED.setBrightness(StripEngineConfig::Brightness);
  pinMode(StripEngineConfig::LeftButtonPin, INPUT_PULLUP);
  pinMode(StripEngineConfig::RightButtonPin, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  clear();
  show();
}

void StripEngine::update()
{
  if (activeGame_ == nullptr)
    return;
  activeGame_->update();
  activeGame_->render();
  show();
}

void StripEngine::mountGame(Game &game)
{
  if (activeGame_ == &game)
    return;
  if (activeGame_ != nullptr)
    activeGame_->onUnmount();
  activeGame_ = &game;
  clear();
  activeGame_->onMount(*this);
}

bool StripEngine::buttonIsPressed(Button button) const
{
  const uint8_t pin = button == Button::Left ? StripEngineConfig::LeftButtonPin : StripEngineConfig::RightButtonPin;
  return digitalRead(pin) == LOW;
}

void StripEngine::show() { FastLED.show(); }
void StripEngine::clear() { fill_solid(leds_, StripEngineConfig::LedCount, CRGB::Black); }
