#include "sonyremote.h"

uint8_t SonyRemote::handleRequestRemoteCapabilitiesPacket(uint8_t &sum, RemoteEvent *event){
  uint8_t block = readDataByte(sum);
  event->type = EventType::NONE;
  prepareRemoteCapabilities(block);
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

    uint8_t lcdType = *lcdBuffer;
    switch(lcdType){
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 0x20:
        event->data.lcd.type = EventLCDText::LCDDataType::TIME;
        break;
      case 0x14:
        event->data.lcd.type = EventLCDText::LCDDataType::TRACK_TITLE;
        break;
      case 0x04:
        event->data.lcd.type = EventLCDText::LCDDataType::DISC_TITLE;
        break;
      default:
        event->data.lcd.type = EventLCDText::LCDDataType::UNKNOWN;
    }

    SerialUSB.println("LCD DATA: ");
    for(uint8_t i = 0; i<lcdOffset; i++){
      if(lcdBuffer[i] < 0x20){
        SerialUSB.print("[");
        if(lcdBuffer[i] <= 0xf) SerialUSB.print("0");
        SerialUSB.print(lcdBuffer[i], HEX);
        SerialUSB.print("]");
      }else SerialUSB.print(lcdBuffer[i]);
    }
    SerialUSB.print("\nTYPE: ");
    SerialUSB.println(static_cast<int>(event->data.lcd.type));
    lcdOffset = 0;
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

uint8_t SonyRemote::handleClearLCDRegisters(uint8_t &sum, RemoteEvent *event){
  event->type = EventType::NONE;
  //lcdOffset = 0;
  return 0;
}

void SonyRemote::prepareRemoteCapabilities(uint8_t block){
  if(block != 0x01){
    D("Error unknown capabilitiy parameter: ");
    D(block);
    DN;
    return;
  }
  
  uint8_t sum = 0xc0; //Packet CAPABILITIES
  addByteToSend(0xc0); //Remote
  
  sum ^= 0x01; //block
  addByteToSend(0x01);

  sum ^= 0xff; //0x09; //chars on screen
  addByteToSend(0xff);

  sum ^= 0x00; //unk
  addByteToSend(0x00);

  sum ^= 0x00; //unk
  addByteToSend(0x00);

  sum ^= 0x20;//0x10; //unk
  addByteToSend(0x20);

  sum ^= 0x0C; //12px tall
  addByteToSend(0x0C);

  sum ^= 0x80; //128px wide
  addByteToSend(0x80);

  sum ^= 0x20; //charset kanji?
  addByteToSend(0x20);

  sum ^= 0x10; //unk
  addByteToSend(0x10);

  addByteToSend(sum);
}

