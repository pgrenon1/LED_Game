#pragma once

#include <FastLED.h>

// Hardware configurations
namespace StripEngineConfig
{
    template <uint8_t DataPin, EOrder ColorOrder>
    using LedChipset = WS2811<DataPin, ColorOrder>;
    constexpr uint8_t LedPin = 9;
    constexpr uint16_t LedCount = 90;
    constexpr EOrder ColorOrder = RGB;
    constexpr uint8_t LeftButtonPin = 2;
    constexpr uint8_t RightButtonPin = 3;
    constexpr uint8_t Brightness = 255;
}
