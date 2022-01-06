//#define FAST1306_SHOW_DELTAS

#include <stdint.h>
#include <Adafruit_GFX.h>
#include <avr/pgmspace.h>
#include <Wire.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DELTA_METADATA_SIZE 16

#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH * ((SCREEN_HEIGHT + 7) / 8))

// From the 'Adafruit_SSD1306' library
#define SSD1306_MEMORYMODE 0x20          ///< See datasheet
#define SSD1306_COLUMNADDR 0x21          ///< See datasheet
#define SSD1306_PAGEADDR 0x22            ///< See datasheet
#define SSD1306_SETCONTRAST 0x81         ///< See datasheet
#define SSD1306_CHARGEPUMP 0x8D          ///< See datasheet
#define SSD1306_SEGREMAP 0xA0            ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON 0xA5        ///< Not currently used
#define SSD1306_NORMALDISPLAY 0xA6       ///< See datasheet
#define SSD1306_INVERTDISPLAY 0xA7       ///< See datasheet
#define SSD1306_SETMULTIPLEX 0xA8        ///< See datasheet
#define SSD1306_DISPLAYOFF 0xAE          ///< See datasheet
#define SSD1306_DISPLAYON 0xAF           ///< See datasheet
#define SSD1306_COMSCANINC 0xC0          ///< Not currently used
#define SSD1306_COMSCANDEC 0xC8          ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET 0xD3    ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  ///< See datasheet
#define SSD1306_SETPRECHARGE 0xD9        ///< See datasheet
#define SSD1306_SETCOMPINS 0xDA          ///< See datasheet
#define SSD1306_SETVCOMDETECT 0xDB       ///< See datasheet

#define SSD1306_SETLOWCOLUMN 0x00  ///< Not currently used
#define SSD1306_SETHIGHCOLUMN 0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE 0x40  ///< See datasheet

#define SSD1306_EXTERNALVCC 0x01  ///< External display voltage source
#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26              ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27               ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A  ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL 0x2E                    ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL 0x2F                      ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3             ///< Set scroll range

typedef unsigned long int ul;
typedef uint8_t byte;

struct Delta{
  uint8_t x;
  uint8_t y;
  uint8_t endX;
  uint8_t endY;
};

class Fast1306Base : public Adafruit_GFX{
  protected:
  virtual void transmitByte(uint8_t b) = 0;
  virtual void transmit(uint8_t *bytes, uint16_t n) = 0;
  virtual void beginTransmission(uint8_t cdc) = 0;
  virtual void endTransmission() = 0;
  void commandList(const uint8_t *c, uint8_t n);
  void command(uint8_t command);
  uint8_t *screenBuffer;
  Delta *deltaMetadata;

  public:
  void display(volatile bool *interrupt);
  void display();
  void begin();
  void beginDelta();
  void endDelta();
  void clearScreen();
  
  void setBounds(uint8_t sX, uint8_t sY, uint8_t eX, uint8_t eY);
  void clearBounds();
  
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  Fast1306Base();

  virtual ~Fast1306Base();

  void markDeltaAsFull();

  private:
  uint8_t contrast;
  int deltaMetadataFILOOffset = 0;
  // The variables below are counted in pixels, NOT BYTES.
  uint8_t currentDeltaWidth;
  uint8_t currentDeltaHeight;
  uint8_t currentDeltaXStart;
  uint8_t currentDeltaYStart;

  struct {
    uint8_t startX;
    uint8_t startY;
    uint8_t endX;
    uint8_t endY;
  } bounds;

   
  uint8_t interruptedDrawing = 0;
  uint8_t interruptedX = 0;
  Delta interrupted;
};

class WireFast1306 : public Fast1306Base{
    public:
    WireFast1306(TwoWire *wire, uint8_t ad);
    virtual ~WireFast1306();

    protected:
    virtual void transmitByte(uint8_t b);
    virtual void transmit(uint8_t *bytes, uint16_t n);
    virtual void beginTransmission(uint8_t cdc);
    virtual void endTransmission();
    void reloadTransmission();

    int transmitted = 0;
    TwoWire *wire;
    bool existing = false;
    uint8_t addr, prevCDC;
};

inline int16_t clipX(int16_t x){
    return min(SCREEN_WIDTH, max(0, x));
}

inline int16_t clipY(int16_t y){
    return min(SCREEN_HEIGHT, max(0, y));
}
