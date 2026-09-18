#pragma once

#include "Game.h"

class BisouGame : public Game {
 public:
  void update() override;
  void render() override;

 private:
  void onMounted() override;
  static constexpr uint8_t kBackgroundBrightness = 2, kInitialTargetZoneBreathingRate = 28;
  static constexpr uint8_t kTargetZoneMinBrightness = 60, kLeftEndpointGlowRate = 30, kLeftEndpointMinBrightness = 100;
  static constexpr uint8_t kRightEndpointGlowRate = 32, kRightEndpointMinBrightness = 100;
  static constexpr int kInitialTargetZoneSize = 8, kMinTargetZoneSize = 1, kTargetZoneEdgeMargin = 10;
  static constexpr int kTargetZoneBreathingRateStep = 16, kMaxMissStreak = 3, kMissMarkerDurationFrames = 7;
  static constexpr unsigned long kPulseMoveIntervalMs = 15;
  int leftPulsePosition_ = -1, rightPulsePosition_ = -1;
  bool leftPulseActive_ = false, rightPulseActive_ = false;
  int targetZoneStart_ = 0, targetZoneEnd_ = 0, currentTargetZoneSize_ = kInitialTargetZoneSize;
  int currentTargetZoneBreathingRate_ = kInitialTargetZoneBreathingRate, missStreak_ = 0;
  unsigned long lastPulseMoveTime_ = 0;
  bool missMarkerActive_ = false;
  int missMarkerCenter_ = 0, missMarkerFrame_ = 0;
  String serialBuffer_;

  void relocateTargetZone(); void removePulses();
  void handleSerial(); void handleInput();
  void movePulses(); void fadeBackground();
  void checkCollision(); void handleMiss(int center);
  void handleHit(); void drawScene();
  void updateMissMarker(); void runHitAnimation();
  void runProgressResetAnimation();
};
