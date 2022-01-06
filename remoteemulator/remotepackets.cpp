#include "sonyremote.h"

uint8_t SonyRemote::handleRequestRemoteCapabilitiesPacket(uint8_t &sum, RemoteEvent *event){
  uint8_t block = readDataByte(sum);
  actionPendingInNextPacket = PendingAction::REMOTE_CAPABILITIES;
  nextActionState[0] = block;
  event->type = EventType::NONE;
  return 1;
}

uint8_t SonyRemote::handleTrackNumberPacket(uint8_t &sum, RemoteEvent *event){
  uint8_t indicatorShown = readDataByte(sum);
  uint8_t unk2 = readDataByte(sum);
  uint8_t unk3 = readDataByte(sum);
  uint8_t trk = readDataByte(sum);
  event->type = EventType::TRACK_NUMBER;
  event->data.trackNumber.number = trk;
  event->data.trackNumber.indicatorShown = indicatorShown;
  return 4;
}

uint8_t SonyRemote::handleDisplayTextPacket(uint8_t &sum, RemoteEvent *event){
  uint8_t segmentType = readDataByte(sum);
  uint8_t unk = readDataByte(sum);
  uint8_t chars[7];
  // Not using a loop to save time.
  chars[0] = readDataByte(sum);
  chars[1] = readDataByte(sum);
  chars[2] = readDataByte(sum);
  chars[3] = readDataByte(sum);
  chars[4] = readDataByte(sum);
  chars[5] = readDataByte(sum);
  chars[6] = readDataByte(sum);
  
  for(uint8_t potential : chars){
    if(potential == 0xff){
      break;
    }
    lcdBuffer[lcdOffset++] = potential; //toPrintable(potential);
  }
  if(segmentType == 0x01){ //Final segment
    lcdBuffer[lcdOffset] = 0;
    lcdOffset = 0;

    uint8_t lcdType = *lcdBuffer;
    switch(lcdType){
    case 0x20:
      event->data.lcd.type = EventLCDText::LCDDataType::TIME;
      break;
    case 0x14:
      event->data.lcd.type = EventLCDText::LCDDataType::TRACK_TITLE;
      break;
    default:
      event->data.lcd.type = EventLCDText::LCDDataType::UNKNOWN;
    }
    event->type = EventType::LCD_TEXT;
    event->data.lcd.text = lcdBuffer + 1; // Skip type
  }else{
    event->type = EventType::NONE;
  }
  return 9;
}


uint8_t SonyRemote::handleRecordIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::RECORD_INDICATOR;
  event->data.record.enabled = readDataByte(sum) == 0x7f;
  return 1;
}

uint8_t SonyRemote::handleAlarmIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::ALARM_INDICATOR;
  event->data.alarm.enabled = readDataByte(sum) == 0x7f;
  return 1;
}

uint8_t SonyRemote::handleVolumeIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  uint8_t level = readDataByte(sum);
  event->type = EventType::VOLUME_LEVEL;
  event->data.volume.level = level == 0xff ? 30 : level;
  return 1;
}

uint8_t SonyRemote::handlePlaybackModeIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::PLAYBACK_MODE;
  event->data.playbackMode.mode = static_cast<EventPlaybackModeIndicator::PlaybackMode>(readDataByte(sum));
  return 1;
}

uint8_t SonyRemote::handleEQIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::PLAYBACK_MODE;
  event->data.eq.eq = static_cast<EventEQIndicator::EQ>(readDataByte(sum));
  return 1;
}

uint8_t SonyRemote::handleBatteryIndicatorPacket(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::BATTERY_LEVEL;
  event->data.battery.level = static_cast<EventBatteryIndicator::Level>(readDataByte(sum));
  return 1;
}

void SonyRemote::continueRemoteCapabilities(){
  if(nextActionState[0] != 0x01){
    D("Error unknown capabilitiy parameter: ");
    D(nextActionState[0]);
    DN;
    return;
  }
  readDataBit(); // Player
  
  uint8_t sum = 0xc0; //Packet CAPABILITIES
  sendDataByte(0xc0); //Remote
  readDataBit(); //Player
  
  sum ^= 0x01; //block
  sendDataByte(0x01);
  readDataBit();

  sum ^= 0xff; //0x09; //chars on screen
  sendDataByte(0xff);
  readDataBit();

  sum ^= 0x00; //unk
  sendDataByte(0x00);
  readDataBit();

  sum ^= 0x00; //unk
  sendDataByte(0x00);
  readDataBit();

  sum ^= 0x20;//0x10; //unk
  sendDataByte(0x20);
  readDataBit();

  sum ^= 0x0C; //12px tall
  sendDataByte(0x0C);
  readDataBit();

  sum ^= 0x80; //128px wide
  sendDataByte(0x80);
  readDataBit();

  sum ^= 0x20; //charset kanji?
  sendDataByte(0x20);
  readDataBit();

  sum ^= 0x10; //unk
  sendDataByte(0x10);
  readDataBit();

  sendDataByte(sum);
}

