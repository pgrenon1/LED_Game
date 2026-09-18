#pragma once

class StripEngine;

// Implement this interface for every game that can run on StripEngine.
class Game {
 public:
  virtual ~Game() = default;

  void onMount(StripEngine& engine) { engine_ = &engine; onMounted(); }
  void onUnmount() { onUnmounted(); engine_ = nullptr; }
  virtual void update() = 0;
  virtual void render() = 0;

 protected:
  StripEngine& engine() { return *engine_; }

  virtual void onMounted() {}
  virtual void onUnmounted() {}

 private:
  // Non-owning: valid only between onMount() and onUnmount().
  StripEngine* engine_ = nullptr;
};
