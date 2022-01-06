#include "fast1306.h"

Fast1306Base::Fast1306Base():
  Adafruit_GFX(SCREEN_WIDTH, SCREEN_HEIGHT),
  deltaMetadataFILOOffset(0),
  currentDeltaWidth(0),
  currentDeltaHeight(0),
  currentDeltaXStart(0),
  currentDeltaYStart(0)
{}

void Fast1306Base::begin(){
  // From the 'Adafruit_SSD1306' library
  uint8_t vccstate = SSD1306_SWITCHCAPVCC;

  screenBuffer = (uint8_t*) malloc(SCREEN_BUFFER_SIZE);
  deltaMetadata = new Delta[DELTA_METADATA_SIZE];
  memset(screenBuffer, 0, SCREEN_BUFFER_SIZE);
  static const uint8_t PROGMEM init1[] = {SSD1306_DISPLAYOFF,         // 0xAE
                                      SSD1306_SETDISPLAYCLOCKDIV, // 0xD5
                                      0x80, // the suggested ratio 0x80
                                      SSD1306_SETMULTIPLEX}; // 0xA8
  commandList(init1, sizeof(init1));
  command(SCREEN_HEIGHT - 1);

  static const uint8_t PROGMEM init2[] = {SSD1306_SETDISPLAYOFFSET, // 0xD3
                                          0x0,                      // no offset
                                          SSD1306_SETSTARTLINE | 0x0, // line #0
                                          SSD1306_CHARGEPUMP};        // 0x8D
  commandList(init2, sizeof(init2));
  command((vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14);

  static const uint8_t PROGMEM init3[] = {SSD1306_MEMORYMODE, // 0x20
                                          0x00, // 0x0 act like ks0108
                                          SSD1306_SEGREMAP | 0x1,
                                          SSD1306_COMSCANDEC};
  commandList(init3, sizeof(init3));

  uint8_t comPins = 0x02;
  contrast = 0x8F;

  if ((SCREEN_WIDTH == 128) && (SCREEN_HEIGHT == 32)) {
    comPins = 0x02;
    contrast = 0x8F;
  } else if ((SCREEN_WIDTH == 128) && (SCREEN_HEIGHT == 64)) {
    comPins = 0x12;
    contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF;
  } else if ((SCREEN_WIDTH == 96) && (SCREEN_HEIGHT == 16)) {
    comPins = 0x2; // ada x12
    contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF;
  } else {
    // Other screen varieties -- TBD
  }
  command(SSD1306_SETCOMPINS);
  command(comPins);
  command(SSD1306_SETCONTRAST);
  command(contrast);

  command(SSD1306_SETPRECHARGE); // 0xd9
  command((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1);
  static const uint8_t PROGMEM init5[] = {
      SSD1306_SETVCOMDETECT, // 0xDB
      0x40,
      SSD1306_DISPLAYALLON_RESUME, // 0xA4
      SSD1306_NORMALDISPLAY,       // 0xA6
      SSD1306_DEACTIVATE_SCROLL,
      SSD1306_DISPLAYON}; // Main screen turn on
  commandList(init5, sizeof(init5));
  clearBounds();
}

Fast1306Base::~Fast1306Base(){
  delete[] deltaMetadata;
  free(screenBuffer);
}

void Fast1306Base::beginDelta(){
  currentDeltaHeight = 0;
  currentDeltaWidth = 0;
  currentDeltaXStart = SCREEN_WIDTH;
  currentDeltaYStart = SCREEN_HEIGHT;
}

inline uint8_t getByteNum(uint8_t pix, bool floor = false){
  return (pix & (~7)) + (((pix & 7) > 0) * 8 * (1 - (floor * 2))) * ((pix & (~7)) > 0);
}

void Fast1306Base::endDelta(){
  // Wrap the delta up and push it onto the filo
  Delta *current = &deltaMetadata[deltaMetadataFILOOffset++];
  current->endY = getByteNum(currentDeltaHeight);
  current->endX = currentDeltaWidth;
  current->x = currentDeltaXStart;
  current->y = getByteNum(currentDeltaYStart, true);

  #ifdef FAST1306_SHOW_DELTAS
  drawFastHLine(current->x, current->y, current->endX - current->x-1, 1);
  drawFastHLine(current->x, current->endY-1, current->endX - current->x-1, 1);
  drawFastVLine(current->x, current->y, current->endY - current->y-1, 1);
  drawFastVLine(current->endX-1, current->y, current->endY - current->y-1, 1);
  #endif
}

void Fast1306Base::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if(x < bounds.startX || x >= bounds.endX || y < bounds.startY || y >= bounds.endY) return;

  currentDeltaXStart = min(currentDeltaXStart, x);
  currentDeltaYStart = min(currentDeltaYStart, y);
  currentDeltaWidth = max(currentDeltaWidth, x + 1);
  currentDeltaHeight = max(currentDeltaHeight, y + 1);
  if(color == 1){
    screenBuffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y & 0b111));
  }else if (color == 2){
    screenBuffer[x + (y / 8) * SCREEN_WIDTH] ^= (1 << (y & 0b111));
  }else{
    screenBuffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y & 0b111));
  }
}

// From the 'Adafruit_SSD1306' library
void Fast1306Base::drawFastHLine(int16_t x, int16_t y, int16_t w,
                                             uint16_t color) {
  int16_t potentialEnd = x + w;
  if(x < bounds.startX) x = bounds.startX;
  if(x >= bounds.endX) x = bounds.endX-1;
  if(y < bounds.startY) y = bounds.startY;
  if(y >= bounds.endY) y = bounds.endY-1;

  if(potentialEnd >= bounds.endX) potentialEnd = bounds.endX-1;
  w = potentialEnd - x;

  currentDeltaXStart = min(currentDeltaXStart, x);
  currentDeltaYStart = min(currentDeltaYStart, y);
  currentDeltaWidth = max(currentDeltaWidth, x + w);
  currentDeltaHeight = max(currentDeltaHeight, y + 1);

  if ((y >= 0) && (y < SCREEN_HEIGHT)) { // Y coord in bounds?
    if (x < 0) {                  // Clip left
      w += x;
      x = 0;
    }
    if ((x + w) > WIDTH) { // Clip right
      w = (WIDTH - x);
    }
    if (w > 0) { // Proceed only if width is positive
      uint8_t *pBuf = &screenBuffer[(y / 8) * SCREEN_WIDTH + x], mask = 1 << (y & 7);
      switch (color) {
      case 2:
        while (w--) {
          *pBuf++ ^= mask;
        };
        break;
      case 1:
        while (w--) {
          *pBuf++ |= mask;
        };
        break;
      case 0:
        mask = ~mask;
        while (w--) {
          *pBuf++ &= mask;
        };
        break;
      }
    }
  }
}


void Fast1306Base::markDeltaAsFull(){
  currentDeltaXStart = 0;
  currentDeltaYStart = 0;
  currentDeltaWidth = SCREEN_WIDTH;
  currentDeltaHeight = SCREEN_HEIGHT;
}

void Fast1306Base::clearScreen(){
  markDeltaAsFull();
  memset(screenBuffer, 0, SCREEN_BUFFER_SIZE);
}

void Fast1306Base::clearBounds(){
  bounds.startX = bounds.startY = 0;
  bounds.endX = SCREEN_WIDTH - 1;
  bounds.endY = SCREEN_HEIGHT - 1;
}

void Fast1306Base::setBounds(uint8_t sX, uint8_t sY, uint8_t eX, uint8_t eY){
  bounds.startX = sX;
  bounds.startY = sY;
  bounds.endX = eX;
  bounds.endY = eY;
}

// From the 'Adafruit_SSD1306' library
void Fast1306Base::drawFastVLine(int16_t x, int16_t __y,
                                             int16_t __h, uint16_t color) {
  int16_t potentialEnd = __y + __h;
  if(x < bounds.startX) x = bounds.startX;
  if(x >= bounds.endX) x = bounds.endX - 1;
  if(__y < bounds.startY) __y = bounds.startY;
  if(__y >= bounds.endY) __y = bounds.endY - 1;
  if(potentialEnd >= bounds.endY) potentialEnd = bounds.endY - 1;
  __h = potentialEnd - __y;
  
  currentDeltaXStart = min(currentDeltaXStart, x);
  currentDeltaYStart = min(currentDeltaYStart, __y);
  currentDeltaWidth = max(currentDeltaWidth, x + 1);
  currentDeltaHeight = max(currentDeltaHeight, __y + __h);

  if ((x >= 0) && (x < SCREEN_WIDTH)) { // X coord in bounds?
    if (__y < 0) {               // Clip top
      __h += __y;
      __y = 0;
    }
    if ((__y + __h) > SCREEN_HEIGHT) { // Clip bottom
      __h = (HEIGHT - __y);
    }
    if (__h > 0) { // Proceed only if SCREEN_HEIGHT is now positive
      // this display doesn't need ints for coordinates,
      // use local byte registers for faster juggling

      uint8_t y = __y, h = __h;
      uint8_t *pBuf = &screenBuffer[(y / 8) * SCREEN_WIDTH + x];

      // do the first partial byte, if necessary - this requires some masking
      uint8_t mod = (y & 7);
      if (mod) {
        // mask off the high n bits we want to set
        mod = 8 - mod;
        // note - lookup table results in a nearly 10% performance
        // improvement in fill* functions
        // uint8_t mask = ~(0xFF >> mod);
        static const uint8_t PROGMEM premask[8] = {0x00, 0x80, 0xC0, 0xE0,
                                                   0xF0, 0xF8, 0xFC, 0xFE};
        uint8_t mask = pgm_read_byte(&premask[mod]);
        // adjust the mask if we're not going to reach the end of this byte
        if (h < mod)
          mask &= (0XFF >> (mod - h));

        switch (color) {
        case 2:
          *pBuf ^= mask;
          break;
        case 1:
          *pBuf |= mask;
          break;
        case 0:
          *pBuf &= ~mask;
          break;
        }
        pBuf += SCREEN_WIDTH;
      }

      if (h >= mod) { // More to go?
        h -= mod;
        // Write solid bytes while we can - effectively 8 rows at a time
        if (h >= 8) {
            // store a local value to work with
        if (color == 2) {
            // separate copy of the code so we don't impact performance of
            // black/white write version with an extra comparison per loop
            do {
              *pBuf ^= 0xFF; // Invert byte
              pBuf += WIDTH; // Advance pointer 8 rows
              h -= 8;        // Subtract 8 rows from height
            } while (h >= 8);
          } else {
            uint8_t val = color * 255;
            do {
              *pBuf = val;   // Set byte
              pBuf += SCREEN_WIDTH; // Advance pointer 8 rows
              h -= 8;        // Subtract 8 rows from SCREEN_HEIGHT
            } while (h >= 8);
          } 
        }

        if (h) { // Do the final partial byte, if necessary
          mod = h & 7;
          // this time we want to mask the low bits of the byte,
          // vs the high bits we did above
          // uint8_t mask = (1 << mod) - 1;
          // note - lookup table results in a nearly 10% performance
          // improvement in fill* functions
          static const uint8_t PROGMEM postmask[8] = {0x00, 0x01, 0x03, 0x07,
                                                      0x0F, 0x1F, 0x3F, 0x7F};
          uint8_t mask = pgm_read_byte(&postmask[mod]);
          switch (color) {
           case 2:
            *pBuf ^= mask;
            break;
          case 1:
            *pBuf |= mask;
            break;
          case 0:
            *pBuf &= ~mask;
            break;
          }
        }
      }
    } // endif positive SCREEN_HEIGHT
  }   // endif x in bounds
}

void Fast1306Base::commandList(const uint8_t *c, uint8_t n){
    beginTransmission(0x00);
    for(int i = 0; i<n; i++) transmitByte(pgm_read_byte(c++));
    endTransmission();
}

void Fast1306Base::command(uint8_t command){
  beginTransmission(0x00);
  transmitByte(command);
  endTransmission();
}

void Fast1306Base::display(){
  bool _ = false;
  display(&_);
}

void Fast1306Base::display(volatile bool *interrupt){
  ul drawingStart = micros();
  while(!*interrupt && (deltaMetadataFILOOffset > 0 || interruptedDrawing)){
    if(!interruptedDrawing){
      //Ok, new delta.
      Delta *last = &deltaMetadata[--deltaMetadataFILOOffset];
      last->y >>= 3; //Convert pixels ==> bytes
      last->endY >>= 3;
      memcpy(&interrupted, last, sizeof(Delta)); 
      // Work on this one in case of an interrupt
      // Using memcpy instead of a raw pointer in case between display calls,
      // a new delta gets pushed onto the queue. As the offset to the end of
      // the queue is decremented, this delta gets overwritten with the new
      // one.
    }
    command(SSD1306_PAGEADDR);
    command(interrupted.y);
    command(interrupted.endY - 1);
    
    if(interruptedDrawing == 2){
      command(SSD1306_COLUMNADDR);
      command(interruptedX);
      command(interrupted.endX - 1);
      beginTransmission(0x40);
      for(; interruptedX < interrupted.endX; interruptedX++){
        transmitByte(screenBuffer[interrupted.y * SCREEN_WIDTH + interruptedX]);
      }
      ++interrupted.y;
      endTransmission();
    }

    command(SSD1306_COLUMNADDR);
    command(interrupted.x);
    command(interrupted.endX - 1);
    beginTransmission(0x40);


    for(; interrupted.y < interrupted.endY; interrupted.y++){
      for(uint8_t x = interrupted.x; x < interrupted.endX; x++){
        if(*interrupt){
          interruptedDrawing = 2;
          endTransmission();
          interruptedX = x;
          return;
        }
        transmitByte(screenBuffer[interrupted.y * SCREEN_WIDTH + x]);        
      }
    }
    interruptedDrawing = 0;
    endTransmission();
  }
}

/***********Communication-dependent subclasses of the Fast1306 Base***********/

void WireFast1306::transmitByte(uint8_t b){
  transmitted += 1;
  wire->write(b);
  if(transmitted >= 32){
    reloadTransmission();
  }
}
void WireFast1306::transmit(uint8_t *b, uint16_t n){
  for(uint16_t i = 0; i<n; i++) transmitByte(*b++);
}

void WireFast1306::reloadTransmission(){
  endTransmission();
  beginTransmission(prevCDC);
}

WireFast1306::WireFast1306(TwoWire *wire, uint8_t ad){
  addr = ad;
  this->wire = wire;
}

void WireFast1306::beginTransmission(uint8_t cdc){
  if(existing) endTransmission();
  existing = true;
  transmitted = 1;
  prevCDC = cdc;
  wire->beginTransmission(addr);
  wire->write(cdc);
}
void WireFast1306::endTransmission(){
  existing = false;
  wire->endTransmission();
}

WireFast1306::~WireFast1306(){}
