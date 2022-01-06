#include "uitools.h"

UiToolkit::UiToolkit(ui w, ui h){
  screenWidth = w;
  screenHeight = h;
}

void UiToolkit::begin(Adafruit_GFX *gfx){
  this->gfx = gfx;
}

void UiToolkit::getTextSize(const char *text, ui *width, ui *height){
  int16_t a, b;
  int16_t x = 0, y = 0;
  uint16_t w, h;
  gfx->getTextBounds(text, x, y, &a, &b, &w, &h);
  if(width) *width = w;
  if(height) *height = h;
}

void UiToolkit::paintDialog(char* text, ui nbuttons){
  //Paint dialog overlay.
  ui textWidth;
  getTextSize(text, &textWidth, NULL);

  ui length = strlen(text);
  ui width = max(2 * MARGIN + textWidth, 60 * nbuttons);

  ui xstart = (screenWidth - width) / 2;
  ui ystart = (screenHeight - DIALOG_HEIGHT) / 2;


  ui textStart = (screenWidth - textWidth) / 2;

  gfx->drawRoundRect(xstart, ystart, width, DIALOG_HEIGHT, screenHeight / 4, 1);
  gfx->setCursor(textStart, ystart + MARGIN);
  gfx->println(text);
}

void UiToolkit::paintDialogButton(const char* text, ui number, ui nbuttons, bool selected){ 
  ui dialogStartY = (screenHeight - DIALOG_HEIGHT) / 2;
  ui textWidth;
  getTextSize(text, &textWidth, NULL);

  int marginAmt = ((int) nbuttons) - 1;

  ui buttonsStartX = (screenWidth - (BUTTON_WIDTH * nbuttons + BUTTON_MARGIN * max(0, marginAmt))) / 2;

  ui startX = buttonsStartX + (number * (BUTTON_WIDTH + BUTTON_MARGIN));
  ui startY = dialogStartY + DIALOG_HEIGHT - MARGIN - BUTTON_HEIGHT;

  gfx->setCursor(startX + ((BUTTON_WIDTH - textWidth) / 2), startY + 2);
  if(selected){
    gfx->fillRoundRect(startX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, 20, 1);
    gfx->setTextColor(0);
  }else{
    gfx->fillRoundRect(startX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, 20, 0);
    gfx->drawRoundRect(startX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, 20, 1);
    gfx->setTextColor(1);
  }
  gfx->println(text);
}



void UiToolkit::initMenu(const char **menu, ui length){
  previousSelectedItem = -1;
  menuSelectedItem = 0;
  menuReadingOffset = 0;
  this->menu = menu;
  menuLength = length;
  redrawWholeMenu();
}

void UiToolkit::redrawWholeMenu(){
  SerialUSB.print("Redrawing menu. Offset=");
  SerialUSB.println(menuReadingOffset);
  gfx->fillRect(0, 0, screenWidth, screenHeight, 0);
  gfx->setTextSize(2);
  gfx->setTextColor(1);
  for(ui i = menuReadingOffset; i<min(LIST_ITEMS_ON_SCREEN, menuLength - menuReadingOffset) + menuReadingOffset; i++){
    gfx->setCursor(LIST_BORDER_MARGIN, (LIST_BORDER_MARGIN + LIST_TEXT_HEIGHT) * (i - menuReadingOffset) + LIST_BORDER_MARGIN + (LIST_BORDER_MARGIN / 2));
    gfx->print(menu[i]);
  }
  gfx->setTextSize(1);
  previousSelectedItem = -1;
  renderMenu();
}

void UiToolkit::menuSelect(){
  currentAnimation = Animation::MENU_SELECT;
  animationTime = micros();
  animationState = 0;
}

inline int16_t getMenuItemOffset(int16_t item){
  return (LIST_BORDER_MARGIN + LIST_TEXT_HEIGHT) * item + LIST_BORDER_MARGIN;
}

void UiToolkit::animate(){
  switch(currentAnimation){
  case Animation::MENU_SELECT:
    if((micros() - animationTime) > ANIMATION_MENU_SELECT_DELAY){
      if(animationState++ < 4){
        animationTime = micros();
        gfx->fillRect(0, getMenuItemOffset(menuSelectedItem - menuReadingOffset), screenWidth, LIST_TEXT_HEIGHT + LIST_BORDER_MARGIN, 2);
      }else currentAnimation = Animation::NONE;
    }
    break;
  }
}

bool UiToolkit::hasPendingAnimations(){ return currentAnimation != Animation::NONE; }

void UiToolkit::drawScrollIndicator(ui x, ui y, bool inverted){
  ui r1 = 0;
  ui r2 = 1;
  ui r3 = 2;
  ui r4 = 3;
  ui r5 = 4;

  if(inverted){
    r1 = 4;
    r2 = 3;
    r3 = 2;
    r4 = 1;
    r5 = 0;
  }

  gfx->drawPixel(x + 2, y + r1, 2);

  gfx->drawFastHLine(x + 1, y + r2, 3, 2);
  gfx->drawFastHLine(x, y + r3, 5, 2);
  gfx->drawFastHLine(x, y + r4, 5, 2);
}

void UiToolkit::renderMenu(){
  if(menuSelectedItem >= (LIST_ITEMS_ON_SCREEN + menuReadingOffset)){
    menuReadingOffset++;
    redrawWholeMenu();
    return; //Will get called again.
  }
  if(menuSelectedItem < menuReadingOffset){
    menuReadingOffset--;
    redrawWholeMenu();
    return;
  }
  
  int16_t offsetOnScreen = menuSelectedItem - menuReadingOffset;

  ui scrollIndicatorX = screenWidth - 5 - 2;
  ui scrollIndicatorY1 = 2 + LIST_BORDER_MARGIN;
  ui scrollIndicatorY2 = screenHeight - 7;


  ui height = LIST_TEXT_HEIGHT + LIST_BORDER_MARGIN;
  if(previousSelectedItem != -1){
    int16_t prevOffsetOnScreen = previousSelectedItem - menuReadingOffset;
    height *= 2;
    offsetOnScreen = min(offsetOnScreen, prevOffsetOnScreen);
    previousSelectedItem = -1;
  }
  gfx->fillRect(0, getMenuItemOffset(offsetOnScreen), screenWidth, height, 2);

  if(menuLength > LIST_ITEMS_ON_SCREEN){
    offsetOnScreen = menuSelectedItem - menuReadingOffset;
    gfx->fillRect(scrollIndicatorX, scrollIndicatorY1, 5, 5, offsetOnScreen == 0);
    gfx->fillRect(scrollIndicatorX, scrollIndicatorY2, 5, 5, offsetOnScreen == (LIST_ITEMS_ON_SCREEN - 1));

    if(menuReadingOffset != 0) drawScrollIndicator(scrollIndicatorX, scrollIndicatorY1, false);
    if((menuReadingOffset + LIST_ITEMS_ON_SCREEN) < menuLength) drawScrollIndicator(scrollIndicatorX, scrollIndicatorY2, true);
  }
}

void UiToolkit::nextMenuItem(){
  if(menuSelectedItem == menuLength - 1) return;
  previousSelectedItem = menuSelectedItem++;
  renderMenu();
}

void UiToolkit::prevMenuItem(){
  if(menuSelectedItem == 0) return;
  previousSelectedItem = menuSelectedItem--;
  renderMenu();
}