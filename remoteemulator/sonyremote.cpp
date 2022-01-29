#include "sonyremote.h"

SonyRemote::SonyRemote(){}
SonyRemote::~SonyRemote(){}

inline bool inRange(ul mn, ul mx, ul v){ 
  return v > mn && v < mx; 
}

void SonyRemote::addByteToSend(uint8_t b){
  addBitToSend(b & 0b00000001);
  addBitToSend(b & 0b00000010);
  addBitToSend(b & 0b00000100);
  addBitToSend(b & 0b00001000);
  addBitToSend(b & 0b00010000);
  addBitToSend(b & 0b00100000);
  addBitToSend(b & 0b01000000);
  addBitToSend(b & 0b10000000);
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
    case 0x08:
      return handleClearLCDRegisters(sum, event);
    default:
      return 255;
  }
}

bool SonyRemote::hasChecksumError(){ return checksumError; }
RemoteEvent* SonyRemote::nextEvent(){
  if(eventsLeft > 0) return &events[eventsLeft-- -1];
  else return NULL;
}


void SonyRemote::finaliseOutboundMessage(){}

SynchronousSonyRemote::SynchronousSonyRemote(int readPin, int writePin) : 
  readPin(readPin), 
  writePin(writePin){}
SynchronousSonyRemote::~SynchronousSonyRemote(){}
/**************************LOW LEVEL HELPER FUNCTIONS*************************/

inline void SynchronousSonyRemote::pin(bool value){
  digitalWrite(writePin, value);
}

inline bool SynchronousSonyRemote::isPin(){
  return digitalRead(readPin);
}

inline void SynchronousSonyRemote::waitFor(bool state){
  while(isPin() != state);
}

inline void SynchronousSonyRemote::pullFor(ul duration, bool state){
  bool current = isPin();
  pin(state);
  delayus(duration);
  pin(current);
}

inline ul SynchronousSonyRemote::getLengthOfPulse(bool state){
  waitFor(state);
  ul then = micros();
  waitFor(!state);
  ul now = micros();
  return now - then;
}

void SynchronousSonyRemote::sendDataBit(bool data){
  waitFor(HIGH);
  pin(data);
  delayus(DATA_DURATION);
  pin(LOW);
}

inline bool SynchronousSonyRemote::readDataBit(){
  waitFor(LOW);
  ul duration = getLengthOfPulse(LOW);
  return !inRange(DATABIT_LOW_RANGE, duration);
}

void SynchronousSonyRemote::handleMessage(ul presyncOffset){
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
  }else if(bitsToSend > 0 && cedeBus){
    continuePreviousMessage();
  }
}

void SynchronousSonyRemote::addBitToSend(bool b){
  outboundBuffer[bitsToSend >> 3] |= (b << (bitsToSend & 0b111));
  ++bitsToSend;
}

inline void SynchronousSonyRemote::handleRemoteHeader(){
  // Nothing yet...
  // 0 <readyForText> 0 0 <hasDataToSend> 0 <fullWidthSupported> <initialized>
  sendDataBit(LOW);
  sendDataBit(isReadyForText);
  sendDataBit(LOW);
  sendDataBit(LOW);
  sendDataBit(bitsToSend > 0);
  sendDataBit(LOW);
  sendDataBit(LOW);
  sendDataBit(isInitialized);
}

inline void SynchronousSonyRemote::continuePreviousMessage(){
  uint8_t bytesToSend = bitsToSend >> 3;
  for(uint8_t i = 0; i<bytesToSend; i++){
    readDataBit();
    sendDataByte(outboundBuffer[i]);
  }
  bitsToSend &= 0b111;
  if(bitsToSend){
    readDataBit();
    uint8_t i = 0;
    while(bitsToSend--){
      sendDataBit(outboundBuffer[bytesToSend] & (1 << i++));
    }
  }
}

void SynchronousSonyRemote::sendDataByte(uint8_t b){
  sendDataBit(b & 0b00000001);
  sendDataBit(b & 0b00000010);
  sendDataBit(b & 0b00000100);
  sendDataBit(b & 0b00001000);
  sendDataBit(b & 0b00010000);
  sendDataBit(b & 0b00100000);
  sendDataBit(b & 0b01000000);
  sendDataBit(b & 0b10000000);
}


inline void SynchronousSonyRemote::resetCommunications(){
  // Reset all the variables related to split messages.
  isReadyForText = false;
  bitsToSend = 0;
  isInitialized = false;
  D("Reset!\n");
}

inline void SynchronousSonyRemote::initialize(){
  isInitialized = true;
  isReadyForText = true;
}

void SynchronousSonyRemote::waitForMessage(){
  waitFor(LOW);
}

