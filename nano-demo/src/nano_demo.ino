#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <jm_PCF8574.h>
#include "Adafruit_MCP23017.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define ENABLE_LCD
#define ENABLE_MCP1
#define ENABLE_MCP2
#define ENABLE_TFT

#define TFT_DC   2
#define TFT_CS   4
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_CLK  13
#define TFT_RST  9

jm_PCF8574 pcf8574; // I2C address fixed later by begin(...)

#ifdef ENABLE_TFT
  Adafruit_ILI9341 tft = Adafruit_ILI9341(
    TFT_CS,
    TFT_DC,
    TFT_MOSI,
    TFT_CLK,
    TFT_RST,
    TFT_MISO
  );
#endif

#ifdef ENABLE_LCD
  LiquidCrystal_I2C lcd(0x27,20,4);  
#endif

Adafruit_MCP23017 mcp0;

#ifdef ENABLE_MCP1
  Adafruit_MCP23017 mcp1;
#endif

#ifdef ENABLE_MCP2
  Adafruit_MCP23017 mcp2;
#endif

int8_t short_wait = 5;
 
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pcf8574.begin(0x3A);
  pcf8574.pinMode(0, OUTPUT);

  mcp0.begin(0x0);
#ifdef ENABLE_MCP1
  mcp1.begin(0x4);
#endif
#ifdef ENABLE_MCP2
  mcp2.begin(0x5);
#endif
  
  pinMode(LED_BUILTIN, OUTPUT);
  
#ifdef ENABLE_TFT
  tft.begin();
    testText();
#endif
  
#ifdef ENABLE_LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Init.");
#endif
    
    for (int8_t ix = 0; ix < 16; ix++) {
      mcp0.pinMode(ix, OUTPUT);
#ifdef ENABLE_MCP1
      mcp1.pinMode(ix, OUTPUT);
#endif
#ifdef ENABLE_MCP2
      mcp2.pinMode(ix, OUTPUT);
#endif
      mcp0.digitalWrite(ix, LOW);
#ifdef ENABLE_MCP1
      mcp1.digitalWrite(ix, LOW);
#endif
#ifdef ENABLE_MCP2
      mcp2.digitalWrite(ix, LOW);
#endif      
    }
}

const uint16_t MAX = 0u-1u;

void flash() {
  mcp1.writeGPIOAB(MAX);
  delay(150);
  mcp1.writeGPIOAB(0);
  delay(150);
}

void loop() {
  Serial.println(millis());

  routine(1);
  routine(2);
  routine(4);
  routine(8);
  routine(16);
  routine(32);
  routine(16);
  routine(8);
  routine(4);
  routine(2);
  routine(1);        
}

void blip() {
  pcf8574.digitalWrite(0, LOW);

  lcd.setCursor(0,3);
  lcd.print("Trigger!");

  delay(500);
  
  pcf8574.digitalWrite(0, HIGH);

  lcd.setCursor(0,3);
  lcd.print("        ");
}

void routine(int8_t interval) {
    short_wait = interval;
  
    blip();
    fall();
    
    blip();
    small_sweep(); 
    
    blip();
    chase(); 
  
    blip();
    bounce();
  
    blip();
    zigzag();
  
    blip();
    big_sweep(); 
}

void fall() {
  for (int8_t iiix = 0; iiix < 3; iiix++) {
    lcd_time();
    
    for (int8_t iix = 8; iix <= 15; iix++) {
      for (int8_t ix = 15; ix >= iix; ix--) {
        mcp1.digitalWrite(ix, HIGH);
        delay(short_wait);
        if (ix != iix)
          mcp1.digitalWrite(ix, LOW);
      }
    }

    for (int8_t iix = 0; iix <= 7; iix++) {
      for (int8_t ix = 7; ix >= iix; ix--) {
        mcp1.digitalWrite(ix, HIGH);
        delay(short_wait);
        if (ix != iix)
          mcp1.digitalWrite(ix, LOW);
      }
    }
    
    delay(short_wait*2);

    for (int8_t iix = 7; iix >= 0; iix--) {
      mcp1.digitalWrite(iix, LOW);
      mcp1.digitalWrite(iix+8, LOW);
      delay(short_wait);
    }
  }
}

void chase() {
  static uint8_t cell[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                            15, 14, 13, 12, 11, 10, 9, 8 };
  for (uint8_t iix = 0; iix < 3; iix++) {
    lcd_time();
    
    for (uint8_t ix = 0; ix < 16; ix++) {
      uint8_t ix2 = ix + 8;
      ix2 %= 16;
      
      mcp1.digitalWrite(cell[ix], HIGH);
      mcp1.digitalWrite(cell[ix2], HIGH);
      delay(short_wait);
      mcp1.digitalWrite(cell[ix], LOW);
      mcp1.digitalWrite(cell[ix2], LOW);
    }
  }    
}

void small_sweep() {
  for (int8_t iix = 0; iix <= 3; iix++) {
    lcd_time();

    for (int8_t ix = 0; ix <= 15; ix++) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      delay(short_wait);
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
    }

    for (int8_t ix = 14; ix >= 0; ix--) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      delay(short_wait);
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
    }
  }
}

void bounce() {
  for (int8_t iix = 0; iix <= 3; iix++) {
    lcd_time();

    for (int8_t ix = 0; ix <= 8; ix++) {
      #ifdef ENABLE_MCP1
          mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP1
          mcp1.digitalWrite(15-ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(15-ix, true);
      #endif
      delay(short_wait);
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP1
          mcp1.digitalWrite(15-ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(15-ix, false);
      #endif
    }
  
    for (int8_t ix = 7; ix >= 1; ix--) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP1
          mcp1.digitalWrite(15-ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(15-ix, true);
      #endif
      delay(short_wait);
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP1
          mcp1.digitalWrite(15-ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(15-ix, false);
      #endif
    }
  }
}

void zigzag() {
  static uint8_t cell[] = { 0,  8,  9,  1,  2, 10, 11, 3,
                            4,  12, 13, 5,  6, 14, 15, 7 };
  for (uint8_t iix = 0; iix < 3; iix++) {
    lcd_time();
    
    for (uint8_t ix = 0; ix < 16; ix++) {
      uint8_t ix2 = ix + 8;
      ix2 %= 16;

      mcp1.digitalWrite(cell[ix], HIGH);
      mcp1.digitalWrite(cell[ix2], HIGH);
      delay(short_wait*2);
      mcp1.digitalWrite(cell[ix], LOW);
      mcp1.digitalWrite(cell[ix2], LOW);

    }

    for (uint8_t ix = 14; ix > 1; ix--) {
      uint8_t ix2 = ix + 8;
      ix2 %= 16;

      mcp1.digitalWrite(cell[ix], HIGH);
      mcp1.digitalWrite(cell[ix2], HIGH);
      delay(short_wait*3);
      mcp1.digitalWrite(cell[ix], LOW);
      mcp1.digitalWrite(cell[ix2], LOW);
    }
  }    
}

void big_sweep() {
    for (int8_t iix = 0; iix < 3; iix++) {
      lcd_time();
      
      for (int8_t ix = 0; ix <= 15; ix++) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      delay(short_wait);
    }
  
    for (int8_t ix = 0; ix <= 15; ix++) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
      delay(short_wait);
    }

    for (int8_t ix = 15; ix >= 0; ix--) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, true);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, true);
      #endif
      delay(short_wait);
    }
  
    for (int8_t ix = 15; ix >= 0; ix--) {
      #ifdef ENABLE_MCP1
        mcp1.digitalWrite(ix, false);
      #endif
      #ifdef ENABLE_MCP2
        mcp2.digitalWrite(ix, false);
      #endif
      delay(short_wait);
    }
  }
}

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

int freeMemory() {
  int free_memory;

  if((int)__brkval == 0)
     free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);

  return free_memory;
}
  
void lcd_time() {
  int seconds   = (int)(millis()/1000);
  int minutes   = seconds / 60;
  int remainder = seconds % 60;
    
  #ifdef ENABLE_LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(minutes);
    lcd.setCursor(0,1);
    lcd.print(remainder);
    lcd.setCursor(0,2);
    lcd.print(freeMemory());
  #endif

  Serial.println(minutes);
  Serial.println(remainder);
  Serial.println(freeMemory());
  
}

////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_TFT
unsigned long testText() {
  tft.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}
#endif
