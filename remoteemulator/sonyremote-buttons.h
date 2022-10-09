#pragma once
#include <Arduino.h>
#include "MCP4561_DIGI_POT.h"

#define WIPER_ADDR 0
#define CIRCUIT_ADDR 4

#define PRESS_TIME 250000

#define MAX_RESISTANCE 50000
#define QUEUE_LENGTH 32

enum class Button{
  PLAY = 200,
  BACK = 1000,
  SOUND = 2300,
  NEXT = 3650,
  PAUSE = 5160,
  STOP = 7100,
  VOL_DECR = 8400,
  VOL_INCR = 9900,
  TRACK_MARK = 11900,
  PLAY_MODE = 14300,
  DISPLAY_SWITCH = 16700,
  RECORD = 19500
};

class SonyRemoteButtons{
  public:
  virtual ~SonyRemoteButtons();
  void sendButton(Button button);
  void enqueueButton(Button button);
  void clearQueue();
  void tick();
  void begin();
  bool isDepressed();

  protected:
  virtual void enableAndSetResistance(uint16_t resistance) = 0;
  virtual void disable() = 0;

  private:
  Button queue[QUEUE_LENGTH];
  uint8_t queueOffset = 0;
  bool enabled = false;
  bool afterPressDelay = false;
  unsigned long int microsEnabled;
};

class SonyRemoteButtonsMCP4561 : public SonyRemoteButtons{
  public:
  SonyRemoteButtonsMCP4561(uint8_t addr);

  private:
  MCP4561 digitalPot;

  protected:
  virtual void enableAndSetResistance(uint16_t resistance);
  virtual void disable();
};
