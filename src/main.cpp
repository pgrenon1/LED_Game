#include <Arduino.h>

#include "BisouGame.h"
#include "StripEngine.h"

StripEngine engine;
BisouGame bisouGame;

void setup()
{
  engine.begin();
  engine.mountGame(bisouGame);
}

void loop()
{
  engine.update();
}
