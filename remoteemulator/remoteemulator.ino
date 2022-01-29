#include "fast1306.h"
#include <Wire.h>
#include "uitools.h"
#include "bitmaps.h"
#include "sonyremote.h"
#include "sonyremote-buttons.h"

#define SIGNAL_PIN 3
#define SIGNAL_SINK_PIN 2
#define OLED_ADDRESS 0x3c
#define MCP4561_ADDRESS 0x2f

#define MARGIN 5
#define TRACK_TITLE_START 10
#define TRACK_TITLE_HEIGHT 14
#define DISC_TITLE_HEIGHT (TRACK_TITLE_HEIGHT / 2)
#define DISC_TITLE_START (TRACK_TITLE_START + TRACK_TITLE_HEIGHT + MARGIN -2) //-2 is delta / page optimization offset
#define TIME_START (SCREEN_HEIGHT - TRACK_TITLE_HEIGHT - 2)


#define TRACK_TITLE_MAX_X 128
#define SCROLL_TICK_DISTANCE 4
#define SCROLL_SPEED 100000
#define SCROLL_TICKS_WAIT_TO_RESCROLL 5
#define SCROLL_TICKS_WAIT_ON_ZERO 60

#define Serial SerialUSB

#define SEC 1000000

typedef uint16_t ui;
typedef unsigned long int ul;

UiToolkit toolkit(SCREEN_WIDTH, SCREEN_HEIGHT);
WireFast1306 screen(&Wire, OLED_ADDRESS);

AsyncSonyRemote remote(SIGNAL_PIN, SIGNAL_SINK_PIN);
SonyRemoteButtonsMCP4561 buttonsEmu(MCP4561_ADDRESS);
volatile bool interrupted = false;

enum class MainState{
  NONE, HOME, TESTMENU
} programState;
MainState awaitingSwitch;
ui battery;
ui volume = 25;
char trackTitle[64];
char discTitle[64];
char *currentTime = NULL;

/************************************Setup************************************/
inline void setupScreen(){
  Wire.begin();
  Wire.setClock(400000L);
  screen.begin();
  toolkit.begin(&screen);
}

inline void setupState(){
  screen.setTextColor(1);
  programState = MainState::NONE;
  awaitingSwitch = MainState::HOME;
  memset(trackTitle, 0, sizeof(trackTitle));
  memset(discTitle, 0, sizeof(discTitle));
}

inline void setupDebug(){
  Serial.begin(9600);
  Serial.println("Hello");
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  strcpy(trackTitle, "Very Long Test Track");
  strcpy(discTitle, "disc title");
  initScrollParameters();
}

inline void setupRemote(){
  pinMode(SIGNAL_PIN, INPUT);
  pinMode(SIGNAL_SINK_PIN, OUTPUT);
  buttonsEmu.begin();
  remote.begin();
}


void setup() {
  setupScreen();
  setupState();
  setupDebug();
  setupRemote();
  buttonsEmu.enqueueButton(Button::DISPLAY_SWITCH);
}

/*******************************UI Drawing*******************************/

int16_t currentTrackScroll = 0;
int16_t currentDiscScroll = 0;
uint8_t trackTitleScrolledFully = 0, discTitleScrolledFully = 0;

ui trackTitleWidth, discTitleWidth;
ul lastScrollTick = 0;

void drawConstantUI(){
  screen.beginDelta();
  screen.clearScreen();
  screen.drawBitmap(0, 0, SPEAKER_DATA, SPEAKER_WIDTH, SPEAKER_HEIGHT, 1);
  screen.drawBitmap(SCREEN_WIDTH - BATTERY_WIDTH, 0, BATTERY_DATA, BATTERY_WIDTH, BATTERY_HEIGHT, 1);
  screen.drawBitmap(0, TRACK_TITLE_START, SONG_DATA, SONG_WIDTH, SONG_HEIGHT, 1);
  screen.drawBitmap(0, DISC_TITLE_START, DISC_DATA, DISC_WIDTH, DISC_HEIGHT, 1);

  screen.endDelta();
}

void redrawAll(){
  drawVolumeValue();
  drawTrackTitle();
  drawDiscTitle();
  drawCurrentTime();
}

void drawVolumeValue(){
  if(programState != MainState::HOME) return;
  screen.beginDelta();
  screen.setCursor(SPEAKER_WIDTH + 2, 0);
  ui width, height;
  toolkit.getTextSize("000", &width, &height);
  screen.fillRect(SPEAKER_WIDTH + 2, 0, width, height, 0);
  screen.print(volume);
  screen.endDelta();
}

void scrollTexts(){
  if(micros() - lastScrollTick < SCROLL_SPEED) return;
  lastScrollTick = micros();
  if(trackTitleWidth > (TRACK_TITLE_MAX_X - (SONG_WIDTH + 1))){
    if(currentTrackScroll == 0){
      trackTitleScrolledFully = SCROLL_TICKS_WAIT_ON_ZERO;
      currentTrackScroll = -SCROLL_TICK_DISTANCE;
    }
    if(trackTitleScrolledFully){
      --trackTitleScrolledFully;
      if(currentTrackScroll < -SCROLL_TICK_DISTANCE) currentTrackScroll = TRACK_TITLE_MAX_X;
    }else{
      currentTrackScroll -= SCROLL_TICK_DISTANCE;
      drawTrackTitle();
      return;
    }
  }

  if(discTitleWidth > (TRACK_TITLE_MAX_X - (DISC_WIDTH + 1))){
    if(currentDiscScroll == 0){
      discTitleScrolledFully = SCROLL_TICKS_WAIT_ON_ZERO;
      currentDiscScroll = -SCROLL_TICK_DISTANCE;
    }

    if(discTitleScrolledFully){
      --discTitleScrolledFully;
      currentDiscScroll = TRACK_TITLE_MAX_X;
    }else{
      currentDiscScroll -= SCROLL_TICK_DISTANCE;
      drawDiscTitle();
    }
  }
}

void initScrollParameters(){
  screen.setTextSize(2);
  toolkit.getTextSize(trackTitle, &trackTitleWidth, NULL);
  screen.setTextSize(1);
  toolkit.getTextSize(discTitle, &discTitleWidth, NULL);
  currentTrackScroll = 0;
  currentDiscScroll = 0;
  discTitleScrolledFully = 0;
  trackTitleScrolledFully = 0;
}


void drawTrackTitle(){
  if(programState != MainState::HOME) return;
  screen.beginDelta();
  screen.setCursor(SONG_WIDTH + 1 + currentTrackScroll + SCROLL_TICK_DISTANCE, TRACK_TITLE_START);
  screen.setBounds(
    SONG_WIDTH + 1, 
    TRACK_TITLE_START, 
    TRACK_TITLE_MAX_X, 
    TRACK_TITLE_HEIGHT + TRACK_TITLE_START + 3
  );
  screen.fillRect(
    SONG_WIDTH + 1,
    TRACK_TITLE_START, 
    (TRACK_TITLE_MAX_X) - (SONG_WIDTH + 1), 
    TRACK_TITLE_HEIGHT + 3, 
    0
  );

  screen.setTextSize(2);
  trackTitleScrolledFully = ((currentTrackScroll + trackTitleWidth + TRACK_TITLE_MAX_X) < 0) * SCROLL_TICKS_WAIT_TO_RESCROLL;
  screen.print(trackTitle);
  screen.setTextSize(1);
  screen.clearBounds();
  screen.endDelta();
}

void drawDiscTitle(){
  if(programState != MainState::HOME) return;
  screen.beginDelta();
  screen.setCursor(DISC_WIDTH + 1 + currentDiscScroll + SCROLL_TICK_DISTANCE, DISC_TITLE_START);

  screen.fillRect(
    DISC_WIDTH + 1, 
    DISC_TITLE_START,
    TRACK_TITLE_MAX_X,
    DISC_TITLE_HEIGHT+2,
    0
  );

  screen.setBounds(
    DISC_WIDTH + 1, 
    DISC_TITLE_START,
    TRACK_TITLE_MAX_X,
    DISC_TITLE_HEIGHT + DISC_TITLE_START
  );

  discTitleScrolledFully = ((currentDiscScroll + discTitleWidth + TRACK_TITLE_MAX_X) < 0) * SCROLL_TICKS_WAIT_TO_RESCROLL;
  screen.print(discTitle);  
  screen.clearBounds();
  screen.endDelta();
}

void drawCurrentTime(){
  if(programState != MainState::HOME || !currentTime) return;
  screen.beginDelta();
  screen.setTextSize(2);
  ui width, height;
  toolkit.getTextSize(currentTime, &width, &height);
  screen.setBounds((SCREEN_WIDTH - width) / 2, SCREEN_HEIGHT - TRACK_TITLE_HEIGHT - 6, SCREEN_WIDTH, SCREEN_HEIGHT);
  screen.setCursor((SCREEN_WIDTH - width) / 2, TIME_START);
  screen.fillRect((SCREEN_WIDTH - width) / 2, TIME_START, width, height, 0);
  screen.print(currentTime);
  screen.setTextSize(1);
  screen.clearBounds();
  screen.endDelta();
}

/********************************Communication********************************/

bool hasDiscTitle = false, hasTrackTitle = false;
ul lastLCDUpdateTime = 0;
ul lastChangeTime = 0;

uint8_t handleCommunication(EventType *typesRead = NULL){
  EventLCDText::LCDDataType thisType;
  uint8_t eventTypeCounter = 0;
  while(remote.handleMessage()){
    RemoteEvent *event;
    if(remote.hasChecksumError()){
      Serial.println("Checksum error");
      return 0;
    }
    while(event = remote.nextEvent()){  
      if(typesRead) typesRead[eventTypeCounter++] = event->type;
      switch(event->type){
        case EventType::LCD_TEXT:
          thisType = event->data.lcd.type;
          switch(event->data.lcd.type){
            case EventLCDText::LCDDataType::TIME:
              currentTime = event->data.lcd.text;
              lastLCDUpdateTime = micros();
              drawCurrentTime();
              break;
            case EventLCDText::LCDDataType::TRACK_TITLE:
              strcpy(trackTitle, event->data.lcd.text);
              initScrollParameters();
              currentTrackScroll = -SCROLL_TICK_DISTANCE; //Start scrolling immediately
              drawTrackTitle();
              hasTrackTitle = true;
              break;
            case EventLCDText::LCDDataType::DISC_TITLE:
              strcpy(discTitle, event->data.lcd.text);
              initScrollParameters();
              currentDiscScroll = -SCROLL_TICK_DISTANCE; //Start scrolling immediately
              drawDiscTitle();
              hasDiscTitle = true;
              break;
            default:
              SerialUSB.print("Unknown LCD: ");
              SerialUSB.println(static_cast<uint8_t>(event->data.lcd.type));
              break;
          }
          break;
        case EventType::VOLUME_LEVEL:
          volume = (100 * ((uint16_t) event->data.volume.level)) / 30;
          drawVolumeValue();
          break;
      }
    }
    buttonsEmu.tick();
    if(((micros() - lastLCDUpdateTime) > 2*SEC || !hasDiscTitle ||!hasTrackTitle) && (micros() - lastChangeTime) > 3*SEC){
      buttonsEmu.sendButton(Button::DISPLAY_SWITCH);
      lastChangeTime = micros();
    }
  }
  return eventTypeCounter;
}

/*********************************UI Updating*********************************/


inline void doUpdates(){
  switch(programState){
    case MainState::HOME:
      scrollTexts();
      break;
  }
}

inline void switchStates(){
  switch(programState = awaitingSwitch){
    case MainState::HOME:
      drawConstantUI();
      redrawAll();
      break;
  }
  awaitingSwitch = MainState::NONE;
}

void loop() {
  handleCommunication();

  if(toolkit.hasPendingAnimations()){
    screen.beginDelta();
    toolkit.animate();
    screen.endDelta();
  }else if(awaitingSwitch != MainState::NONE){
    switchStates();    
  }else {
    doUpdates();
  }
  screen.display();
}

