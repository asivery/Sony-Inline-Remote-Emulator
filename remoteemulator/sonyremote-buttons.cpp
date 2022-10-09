#include "sonyremote-buttons.h"

void SonyRemoteButtons::sendButton(Button button){
  uint16_t res = static_cast<uint16_t>(button);
  uint16_t resistance = (res * 257) / MAX_RESISTANCE;
  enableAndSetResistance(resistance);
  microsEnabled = micros();
  enabled = true;
}

void SonyRemoteButtons::enqueueButton(Button button){
  if(queueOffset >= QUEUE_LENGTH) return;
  queue[queueOffset++] = button;
}
void SonyRemoteButtons::begin(){ disable(); }

void SonyRemoteButtons::tick(){
  if(!isDepressed() && queueOffset){
    SerialUSB.print("Q: ");
    SerialUSB.print(queueOffset);
    SerialUSB.print(" ");
    SerialUSB.println(static_cast<int>(queue[0]));
    sendButton(queue[0]);
    for(uint8_t i = 1; i < queueOffset; i++){
      queue[i - 1] = queue[i];
    }
    --queueOffset;
  }

  if((micros() - microsEnabled) > PRESS_TIME){
    if(enabled){
      enabled = false;
      microsEnabled = micros();
      afterPressDelay = true;
      disable();
    }else if(afterPressDelay){
      afterPressDelay = false;
    }
  }
}

bool SonyRemoteButtons::isDepressed(){ return enabled || afterPressDelay; }

SonyRemoteButtons::~SonyRemoteButtons(){}

void SonyRemoteButtons::clearQueue(){ queueOffset = 0; }

SonyRemoteButtonsMCP4561::SonyRemoteButtonsMCP4561(uint8_t addr): digitalPot(addr){  }

void SonyRemoteButtonsMCP4561::disable(){
  digitalPot.potDisconnectAll(MCP4561_WIPER_0);
}

void SonyRemoteButtonsMCP4561::enableAndSetResistance(uint16_t resistance){
  digitalPot.writeVal(MCP4561_VOL_WIPER_0, resistance);
  digitalPot.potConnectAll(MCP4561_WIPER_0);
}
