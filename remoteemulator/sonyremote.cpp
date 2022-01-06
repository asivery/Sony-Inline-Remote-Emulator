#include "sonyremote.h"

SonyRemote::SonyRemote(int readPin, int writePin){
  this->readPin = readPin;
  this->writePin = writePin;
}
/**************************LOW LEVEL HELPER FUNCTIONS*************************/

inline bool inRange(ul mn, ul mx, ul v){ 
  return v > mn && v < mx; 
}

inline void SonyRemote::pin(bool value){
  digitalWrite(writePin, value);
}

inline bool SonyRemote::isPin(){
  return digitalRead(readPin);
}

inline void SonyRemote::waitFor(bool state){
  while(isPin() != state);
}

inline void SonyRemote::pullFor(ul duration, bool state){
  bool current = isPin();
  pin(state);
  delayus(duration);
  pin(current);
}

inline ul SonyRemote::getLengthOfPulse(bool state){
  waitFor(state);
  ul then = micros();
  waitFor(!state);
  ul now = micros();
  return now - then;
}

void SonyRemote::sendDataBit(bool data){
  waitFor(HIGH);
  pin(data);
  delayus(DATA_DURATION);
  pin(LOW);
}

void SonyRemote::sendDataByte(uint8_t b){
  sendDataBit(b & 0b00000001);
  sendDataBit(b & 0b00000010);
  sendDataBit(b & 0b00000100);
  sendDataBit(b & 0b00001000);
  sendDataBit(b & 0b00010000);
  sendDataBit(b & 0b00100000);
  sendDataBit(b & 0b01000000);
  sendDataBit(b & 0b10000000);
}

inline bool SonyRemote::readDataBit(){
  waitFor(LOW);
  ul duration = getLengthOfPulse(LOW);
  return !inRange(DATABIT_LOW_RANGE, duration);
}

uint8_t SonyRemote::readDataByte(uint8_t &checksum){
  uint8_t b = 0;
  b |= (readDataBit());
  b |= (readDataBit() << 1);
  b |= (readDataBit() << 2);
  b |= (readDataBit() << 3);
  b |= (readDataBit() << 4);
  b |= (readDataBit() << 5);
  b |= (readDataBit() << 6);
  b |= (readDataBit() << 7);
  checksum ^= b;
  return b;
}

/*******************************MESSAGE PARSING*******************************/

inline void SonyRemote::resetCommunications(){
  // Reset all the variables related to split messages.
  isReadyForText = false;
  actionPendingInNextPacket = PendingAction::NONE;
  isInitialized = false;
  D("Reset!\n");
}

inline void SonyRemote::initialize(){
  isInitialized = true;
  isReadyForText = true;
}

void SonyRemote::waitForMessage(){
  waitFor(LOW);
}

void SonyRemote::handleMessage(ul presyncOffset){
  ul presyncDuration = presyncOffset + getLengthOfPulse(LOW);
  eventsLeft = 0;
  checksumError = false;
  #ifdef PRECISE_PRESYNC
  if(!inRange(PRESYNC_RANGE, presyncDuration)){
  #endif
    if(inRange(PRESYNC_RESET_RANGE, presyncDuration)){
      resetCommunications();
    }
    #ifdef PRECISE_PRESYNC
    else{
      D("Not in presync range: ");
      D(presyncDuration);
      DN;
      event->type = EventType::TIMING_ERROR;
      return;
    }
  }
  #endif
  // Now wait for the presync delay to pass
  waitFor(LOW);

  // Now in SYNC pulse
  ul syncDuration = getLengthOfPulse(LOW);
  if(!inRange(SYNC_RANGE, syncDuration)){
    // Nope - something's wrong;
    D("Not in sync range: ");
    D(syncDuration);
    DN;
    return;
  }
  // Is now HIGH

  // Now in the remote header - we need to send data to inform the player 
  // about the remote's state
  handleRemoteHeader();

  // Now in player header - read...

  waitFor(HIGH);

  bool hasData = !readDataBit();
  bool unk1 = readDataBit();
  bool unk2 = readDataBit();
  bool unk3 = readDataBit();
  bool cedeBus = readDataBit();
  bool unk4 = readDataBit();
  bool unk5 = readDataBit();
  bool playerPresent = readDataBit(); //It's not like we're getting this data from the player...

  if(!isInitialized){
    initialize();
    D("INIT!\n");
  }else if(!cedeBus && hasData){
    //Player's talking
    handlePlayerMessage(); 
  }else if(actionPendingInNextPacket != PendingAction::NONE){
    continuePreviousMessage();
  }
}

inline void SonyRemote::handleRemoteHeader(){
  // Nothing yet...
  // 0 <readyForText> 0 0 <hasDataToSend> 0 <fullWidthSupported> <initialized>
  sendDataBit(LOW);
  sendDataBit(isReadyForText);
  sendDataBit(LOW);
  sendDataBit(LOW);
  sendDataBit(actionPendingInNextPacket != PendingAction::NONE);
  sendDataBit(LOW);
  sendDataBit(LOW);
  sendDataBit(isInitialized);
}

inline void SonyRemote::handlePlayerMessage(){
  eventsLeft = 0;
  uint8_t bytesRead = 0;
  uint8_t sum = 0;
  while(bytesRead < 10){
    uint8_t type = readDataByte(sum);
    ++bytesRead;
    if(type == 0){
      break; //No more data to read from this message
    }
    uint8_t bytesReadFromPacket = handlePlayerPacket(type, sum, &events[eventsLeft++]);
    if(events[eventsLeft - 1].type == EventType::NONE) eventsLeft--; // overwrite the 'NONE' event
    if(bytesReadFromPacket == 255 /* unknown */){
      D("Error - unknown packet ");
      D(type);
      D(". Because of this ");
      D(9 - bytesRead);
      D(" bytes have been dropped.\n");
      break;
    }
    bytesRead += bytesReadFromPacket;
  }
  while(bytesRead < 10){ // Read the rest in case the message wasn't full
    ++bytesRead;
    readDataByte(sum);
  }
  readDataByte(sum);
  checksumError = sum; // x ^ x = 0, if sum == 0, there are no checksum errors.
}

// Individual packet handling code in 'remotepackets.cpp'
inline uint8_t SonyRemote::handlePlayerPacket(uint8_t type, uint8_t &sum, RemoteEvent *event){
  switch(type){
    case 0x01:
      return handleRequestRemoteCapabilitiesPacket(sum, event);
    case 0xa0:
      return handleTrackNumberPacket(sum, event);
    case 0xc8:
      return handleDisplayTextPacket(sum, event);
    case 0x42:
      return handleRecordIndicatorPacket(sum, event);
    case 0x47:
      return handleAlarmIndicatorPacket(sum, event);
    case 0x40:
      return handleVolumeIndicatorPacket(sum, event);
    case 0x41:
      return handlePlaybackModeIndicatorPacket(sum, event);
    case 0x46:
      return handleEQIndicatorPacket(sum, event);
    case 0x43:
      return handleBatteryIndicatorPacket(sum, event);

    default:
      return 255;
  }
}

// Individual packet handling code in 'remotepackets.cpp'
inline void SonyRemote::continuePreviousMessage(){
  PendingAction thisAction = actionPendingInNextPacket;
  actionPendingInNextPacket = PendingAction::NONE;
  switch(thisAction){
    case PendingAction::REMOTE_CAPABILITIES:
      continueRemoteCapabilities();
      break;
  }
}

bool SonyRemote::hasChecksumError(){ return checksumError; }
RemoteEvent* SonyRemote::nextEvent(){
  if(eventsLeft > 0) return &events[eventsLeft-- -1];
  else return NULL;
}
