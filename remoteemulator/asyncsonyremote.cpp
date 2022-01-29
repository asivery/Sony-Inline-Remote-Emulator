#include "sonyremote.h"

namespace asr{
  volatile bool hasData = false;
  volatile bool isReadyForText = true;
  volatile bool isInitialized = true;

  volatile bool awaitingNextMessage = true;
  volatile ul bitStartTime;

  volatile uint8_t messageBuffer[11];
  volatile uint8_t messageBufferOffset;

  volatile uint8_t outboundBuffer[11];
  volatile bool hasMessageToSend;

  volatile uint8_t completeMessageBuffer[11 * 10];
  volatile uint8_t completeMessageOffset = 0;

  volatile enum class TransmitState{
    AWAITING_MESSAGE, BEFORE_SYNC, IN_PLAYER_HEADER, IN_REMOTE_HEADER, PLAYER_SENDING, REMOTE_SENDING
  } state;

  volatile uint8_t playerHeaderFlags;

  volatile uint8_t writePin;
  volatile uint8_t readPin;

  inline bool inRange(ul mn, ul mx, ul v){ 
    return v > mn && v < mx; 
  }


  inline bool toBit(unsigned long int time){
    return !inRange(DATABIT_LOW_RANGE, time);
  }

  inline void resetComm(){
    playerHeaderFlags = 0;
    state = TransmitState::AWAITING_MESSAGE;
    messageBufferOffset = 0;
  }

  inline void writeDataBit(bool s){
    if(!s) return;
    digitalWrite(writePin, HIGH);
    delayMicroseconds(DATA_DURATION);
    digitalWrite(writePin, LOW);
  }

  void asyncSonyRemoteISR(){
    if(digitalRead(3) == LOW){
      // falling
      bitStartTime = micros();
      return;
    }
    // Rising - End of bit
    
    unsigned long int duration = micros() - bitStartTime;
    if(duration > 1000000L){
      // Jitter? - unknown condition really...
      return;
    }

    digitalWrite(9, state == TransmitState::IN_REMOTE_HEADER); //DEBUG
    digitalWrite(8, LOW); //DEBUG

    if(inRange(PRESYNC_RANGE, duration)) {
      state = TransmitState::BEFORE_SYNC;
      digitalWrite(8, HIGH);
      return;
    }
    switch(state){
      case TransmitState::AWAITING_MESSAGE:
        if(inRange(PRESYNC_RANGE, duration)) state = TransmitState::BEFORE_SYNC;
        else resetComm();
        break;
      case TransmitState::BEFORE_SYNC:
        if(inRange(SYNC_RANGE, duration)) {
          state = TransmitState::IN_REMOTE_HEADER;
          messageBufferOffset = 0;
          // fall through
        }
        else{ 
          resetComm(); 
          break;
        }
      case TransmitState::IN_REMOTE_HEADER:
        switch(messageBufferOffset++){
          case 0:
          case 2:
          case 3:
          case 5:
          case 6:
            writeDataBit(0);
            break;
          case 1:
            writeDataBit(isReadyForText);
            break;
          case 4:
            writeDataBit(hasMessageToSend);
            break;
          case 7:
            writeDataBit(isInitialized);
            break;
          case 8:
            messageBufferOffset = 0;
            playerHeaderFlags = 0;
            state = TransmitState::IN_PLAYER_HEADER;
          default:
            break;
        }
        break;
      case TransmitState::IN_PLAYER_HEADER:
        playerHeaderFlags |= (toBit(duration) << messageBufferOffset++);
        if(messageBufferOffset == 8) {
          messageBufferOffset = 0;
          bool hasData = !(playerHeaderFlags & 0x1);
          bool cedeBus = playerHeaderFlags & (1 << 4);
          messageBufferOffset = 0;
          if(hasData && !cedeBus){
            memset((void*) messageBuffer, 0, 11);
            state = TransmitState::PLAYER_SENDING;
            break;
          }else if(cedeBus && hasMessageToSend){
            state = TransmitState::REMOTE_SENDING;
            break;
          }
          state = TransmitState::AWAITING_MESSAGE;
        }
        break;
      case TransmitState::REMOTE_SENDING:
        if(messageBufferOffset < 88){
          writeDataBit(outboundBuffer[messageBufferOffset >> 3] & (1 << (messageBufferOffset & 0b111)));
          ++messageBufferOffset;
        }else{
          hasMessageToSend = false;
          memset((void*) outboundBuffer, 0, 11);
          resetComm();
        }
        break;
      case TransmitState::PLAYER_SENDING:
        messageBuffer[messageBufferOffset >> 3] |= (toBit(duration) << (messageBufferOffset & 0b111));
        ++messageBufferOffset;
        if(messageBufferOffset >= 88){
          #ifdef REMOTE_DEBUG
          uint8_t csum = 0;
          for(uint8_t i = 0; i<11; i++) csum ^= messageBuffer[i];
          if(csum){
            D("----------CSUM ERR----------\n");
            D(csum);
            DN;
            for(uint8_t i = 0; i<11; i++){
              D(messageBuffer[i], HEX);
              D(" ");
            }
            D("\n----------------------------\n");
          }
          #endif
          if(completeMessageOffset < 10) {
            uint8_t bufferOffset = 11 * completeMessageOffset++;
            for(uint8_t i = 0; i<11; i++) completeMessageBuffer[bufferOffset + i] = messageBuffer[i];          
          }else
            D("Error - queue overflow!");
            DN;
          resetComm();
        }
        break;
    }
    digitalWrite(9, state == TransmitState::IN_REMOTE_HEADER);
  }
  uint8_t readingCursor = 0;
  uint8_t writingCursor = 0;
}

using namespace asr;

AsyncSonyRemote::AsyncSonyRemote(int r, int w){
  writePin = w;
  readPin = r;
}

void AsyncSonyRemote::begin(){
  attachInterrupt(digitalPinToInterrupt(readPin), asyncSonyRemoteISR, CHANGE);
}

bool AsyncSonyRemote::readDataBit(){
  bool b = completeMessageBuffer[readingCursor >> 3] & (1 << (readingCursor & 0b111));
  ++readingCursor;
  return b;
}

void AsyncSonyRemote::addBitToSend(bool b){
  outboundBuffer[writingCursor >> 3] |= b << (writingCursor & 0b111);
  ++writingCursor;
}

void AsyncSonyRemote::finaliseOutboundMessage(){
  hasMessageToSend = true;
}

bool AsyncSonyRemote::handleMessage(){
  if(completeMessageOffset){
    readingCursor = 0;
    SerialUSB.println("Handling: ");
    for(int i = 0; i<11; i++){
      SerialUSB.print(completeMessageBuffer[i], HEX);
      SerialUSB.print(" ");
    }
    SerialUSB.println();
    handlePlayerMessage();

    noInterrupts();
    for(uint8_t i = 11; i<110; i++){
      completeMessageBuffer[i - 11] = completeMessageBuffer[i];
    }    
    --completeMessageOffset;
    interrupts();

    return true;
  }
  return false;
}
