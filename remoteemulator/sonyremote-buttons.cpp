#include "sonyremote-buttons.h"

void SonyRemoteButtons::sendButton(Button button){
  uint16_t res = static_cast<uint16_t>(button);
  uint16_t resistance = (res / MAX_RESISTANCE) * 257;
  enableAndSetResistance(resistance);
  microsEnabled = micros();
  enabled = true;
}

void SonyRemoteButtons::tick(){
  if(enabled && ((micros() - microsEnabled) > PRESS_TIME)){
    enabled = false;
    disable();
  }
}

SonyRemoteButtons::~SonyRemoteButtons(){}

SonyRemoteButtonsMCP4561::SonyRemoteButtonsMCP4561(uint8_t addr) : digitalPot(addr){}

void SonyRemoteButtonsMCP4561::disable(){
  digitalPot.potDisconnectAll(MCP4561_WIPER_0);
}

void SonyRemoteButtonsMCP4561::enableAndSetResistance(uint16_t resistance){
  digitalPot.writeVal(MCP4561_VOL_WIPER_0, resistance);
  digitalPot.potConnectAll(MCP4561_WIPER_0);
}
