#pragma once

//#define REMOTE_DEBUG
//#define PRECISE_PRESYNC

#include <stdint.h>
#include <Arduino.h>

/*************************TIMINGS********************/
#define DATA_DURATION 210 /*us*/
#define PRESYNC_RESET_RANGE 39000, 45000 /*Was 41000*/
#define PRESYNC_RANGE 900, 1600 /*Was 1120*/
#define SYNC_RANGE 190, 250 /*Was 220*/
#define DATABIT_LOW_RANGE 100, 250

  
#define delayus delayMicroseconds
#define MAX_PACKETS_PER_MESSAGE 10 /*Packets are 11 bytes, 1 for checksum. There can be max. 10 1 byte packets.*/

#ifdef REMOTE_DEBUG
#define D(x...) SerialUSB.print(x)
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

class SonyRemote{
  public:
  SonyRemote();
  ~SonyRemote();

  RemoteEvent* nextEvent();
  bool hasChecksumError();

  protected:
  // Low-level helpers:

  virtual bool readDataBit() = 0;
  virtual void addBitToSend(bool b) = 0;
  virtual uint8_t readDataByte(uint8_t &checksum);
  virtual void addByteToSend(uint8_t byte);
  virtual void finaliseOutboundMessage();

  // Communication methods:
  void handlePlayerMessage();
  uint8_t handlePlayerPacket(uint8_t type, uint8_t &sum, RemoteEvent *event);

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
  uint8_t handleClearLCDRegisters(uint8_t &sum, RemoteEvent *event);

  void prepareRemoteCapabilities(uint8_t block);

  RemoteEvent events[MAX_PACKETS_PER_MESSAGE]; 
  uint8_t eventsLeft = 0;
  bool checksumError;

  char lcdBuffer[64];
  uint8_t lcdOffset = 0;
};

class SynchronousSonyRemote : public SonyRemote{
  public:
  SynchronousSonyRemote(int readPin, int writePin);
  virtual ~SynchronousSonyRemote();
  void handleMessage(ul presyncOffset = 0);
  void waitForMessage();

  protected:
  uint8_t bitsToSend;
  uint8_t outboundBuffer[11];
  int readPin, writePin;
  void pin(bool value);
  bool isPin();
  void waitFor(bool state);
  void pullFor(ul duration, bool state);
  ul getLengthOfPulse(bool state);
  void sendDataBit(bool data);
  void sendDataByte(uint8_t data);
  void continuePreviousMessage();
  void handleRemoteHeader();
  void resetCommunications();
  void initialize();


  virtual bool readDataBit();
  virtual void addBitToSend(bool b);


  bool hasDataToSend = false;
  bool isReadyForText = false;
  bool isInitialized = false;

};

class AsyncSonyRemote : public SonyRemote{

  public:
  AsyncSonyRemote(int readPin, int writePin);
  bool handleMessage();
  void begin();

  protected:
  virtual bool readDataBit();
  virtual void addBitToSend(bool b);
  virtual void finaliseOutboundMessage();
};

int repr(char* buffer, int bufferLength, RemoteEvent* event);
