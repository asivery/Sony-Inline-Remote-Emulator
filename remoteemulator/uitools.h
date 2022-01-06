#pragma once
#include <Adafruit_GFX.h>
#include <stdint.h>

#define MARGIN 5
#define BUTTON_MARGIN 5
#define BUTTON_WIDTH 50
#define BUTTON_HEIGHT 10
#define DIALOG_HEIGHT 40

#define LIST_BORDER_MARGIN 4
#define LIST_TEXT_HEIGHT 16
#define LIST_ITEMS_ON_SCREEN 3

#define ANIMATION_MENU_SELECT_DELAY 60000

typedef uint16_t ui;

class UiToolkit{
  public:
  UiToolkit(ui w, ui h);

  void begin(Adafruit_GFX *gfx);
  void paintDialog(char* text, ui nbuttons);
  void paintDialogButton(const char* text, ui number, ui max, bool selected);
  void paintButton(ui x, ui y, const char* text);
  void getTextSize(const char *text, ui *width, ui *height);

  void initMenu(const char **menu, ui length);
  void nextMenuItem();
  void prevMenuItem();
  void menuSelect();

  bool hasPendingAnimations();
  void animate();

  protected:

  enum class Animation{
    NONE, MENU_SELECT
  } currentAnimation;

  unsigned long int animationTime;
  ui animationState;

  Adafruit_GFX *gfx;
  ui screenWidth;
  ui screenHeight;


  int16_t menuSelectedItem;
  ui menuReadingOffset;
  ui menuLength;
  
  int16_t previousSelectedItem;

  void redrawWholeMenu();
  void renderMenu();
  void drawScrollIndicator(ui x, ui y, bool inverted);

  const char **menu;
};