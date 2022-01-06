#pragma once

#define REMOTE_DEBUG
//#define PRECISE_PRESYNC

#include <stdint.h>
#include <Arduino.h>

/*************************TIMINGS********************/
#define DATA_DURATION 210 /*us*/
#define PRESYNC_RESET_RANGE 39000, 45000 /*Was 41000*/
#define PRESYNC_RANGE 900, 1300 /*Was 1120*/
#define SYNC_RANGE 190, 250 /*Was 220*/
#define DATABIT_LOW_RANGE 100, 290

  
#define delayus delayMicroseconds
#define MAX_PACKETS_PER_MESSAGE 10 /*Packets are 11 bytes, 1 for checksum. There can be max. 10 1 byte packets.*/

#ifdef REMOTE_DEBUG
#define D(x) SerialUSB.print(x)
#define DN SerialUSB.println()
#else
#define D
#define DN
#endif

typedef unsigned long int ul;

enum class EventType{
  NONE,
  LCD_TEXT, 
  TRACK_NUMBER, 
  VOLUME_LEVEL, 
  BATTERY_LEVEL, 
  ALARM_INDICATOR,
  RECORD_INDICATOR,
  EQ_INDICATOR,
  PLAYBACK_MODE,
  NOT_IMPLEMENTED
};

struct EventTrackNumber{
  uint8_t number;
  bool indicatorShown;
};

struct EventLCDText{
  enum class LCDDataType{
    UNKNOWN, TIME, DISC_TITLE, TRACK_TITLE
  } type;
  char* text;
};

struct EventVolumeIndicator{
  uint8_t level;
};

struct EventBatteryIndicator{
  enum class Level{
    OFF = 0, 
    SINGLE_BAR_BLINKING = 0x01, 
    CHARGING = 0x7f, 
    EMPTY_BLINKING = 0x80, 
    ONE_BAR = 0x9f, 
    TWO_BARS = 0xbf, 
    THREE_BARS = 0xdf, 
    FOUR_BARS = 0xff
  } level;
};

struct EventAlarmIndicator{
  bool enabled;
};

struct EventRecordIndicator{
  bool enabled;
};

struct EventPlaybackModeIndicator{
  enum class PlaybackMode{
    NORMAL = 0,
    REPEAT_ALL = 1,
    ONLY_ONE = 2,
    REPEAT_ONE = 3,
    SHUFFLE = 4,
    SHUFFLE_REPEAT = 5,
    PGM = 6,
    PGM_REPEAT = 7
  } mode;
};

struct EventEQIndicator{
  enum class EQ{
    NORMAL = 0,
    BASS_1 = 1,
    BASS_2 = 2,
    SOUND_1 = 3,
    SOUND_2 = 4
  } eq;
};

struct RemoteEvent{
  EventType type;
  union{
    EventTrackNumber trackNumber;
    EventLCDText lcd;
    EventRecordIndicator record;
    EventAlarmIndicator alarm;
    EventVolumeIndicator volume;
    EventPlaybackModeIndicator playbackMode;
    EventEQIndicator eq;
    EventBatteryIndicator battery;
  } data;
};

enum class PendingAction{
  NONE, REMOTE_CAPABILITIES
};

class SonyRemote{
  public:
  SonyRemote(int readPin, int writePin);

  void handleMessage(ul presyncOffset = 0);
  void waitForMessage();

  RemoteEvent* nextEvent();
  bool hasChecksumError();

  protected:
  int readPin, writePin;
  // Low-level helpers:
  void pin(bool value);
  bool isPin();
  void waitFor(bool state);
  void pullFor(ul duration, bool state);
  ul getLengthOfPulse(bool state);
  void sendDataBit(bool data);
  void sendDataByte(uint8_t data);
  bool readDataBit();
  uint8_t readDataByte(uint8_t &checksum);

  // Communication methods:
  void resetCommunications();
  void initialize();
  void handleRemoteHeader();
  void handlePlayerMessage();
  uint8_t handlePlayerPacket(uint8_t type, uint8_t &sum, RemoteEvent *event);
  void continuePreviousMessage();

  // Packet handling (remotepackets.cpp)
  uint8_t handleRequestRemoteCapabilitiesPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleTrackNumberPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleDisplayTextPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleRecordIndicatorPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleAlarmIndicatorPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleVolumeIndicatorPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handlePlaybackModeIndicatorPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleEQIndicatorPacket(uint8_t &sum, RemoteEvent *event);
  uint8_t handleBatteryIndicatorPacket(uint8_t &sum, RemoteEvent *event);

  void continueRemoteCapabilities();

  RemoteEvent events[MAX_PACKETS_PER_MESSAGE]; 
  uint8_t eventsLeft = 0;
  bool checksumError;

  char lcdBuffer[64];
  uint8_t lcdOffset = 0;
  bool hasDataToSend = false;
  bool isReadyForText = false;
  bool isInitialized = false;
  PendingAction actionPendingInNextPacket = PendingAction::NONE;
  uint8_t nextActionState[16]; //Temp
};