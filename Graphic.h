#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#include "Voice.h" 
// If using software SPI (the default case):
#define OLED_MOSI  PC14 // D1
#define OLED_CLK   PB10 // D0
#define OLED_DC    PB1 // DC
#define OLED_CS    PB0 // CS
#define OLED_RESET PB11 // RES

const int width =  SSD1306_LCDWIDTH; // pixel-width of display
const int height = SSD1306_LCDHEIGHT; // pixel height of display
const int LF = 8;     // line feed size

const int BASE = 0;
const int PAGEMAX = 1;

const int chDispY = LF;
const int noOnY = LF*2;   // note on-off y pos
const int chDispX = 0;   // midi Channel
const int ntOnX = 42;   // note on x pos
const int ntOffX = 85;   // note off x pos
const int curveOffX = 42;
const int ntW = 42; // width of fields

const int ARROW_WIDTH = 8;
const int DGAP = 3;
int WAVEDISP_WIDTH = width - 2*ARROW_WIDTH - 2 * DGAP;
int xstart = ARROW_WIDTH + DGAP;

uint16_t  page = BASE;//SCOPE; //BASE;    // this is the ui-page we actually display
uint16_t  opx = 0;
uint16_t  opy = 0;


bool requestToUpdate = true; // if true, execute the  display.display() function
int cx = 0, cy = 0;

 Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
 
void initGraphic() {
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  //delay(2000);
  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}




/**
 * Quantum FS Start screen
 */
void displayPage0(int freq, int wave) {
  display.clearDisplay();
  
  int h = 22;
  int off = 5;
  // text display tests
  display.setTextSize(2);
  int y = 2;
  int x = off;
  display.setCursor(x, y);
  display.println("FS-FreqGen"); 
  off = 15;
  y += h;
  x = off;
  display.setCursor(x, y);
  // clear old text
  //display.fillRect(x, y, width, h, 0);
  display.print("F=");
  display.print(freq);
  display.println(" Hz");
  y += h;
  x = off;
  display.setCursor(x, y);
  // clear old text
  //display.fillRect(x, y, width, h, 0);
  //display.print("W=");
  switch (wave) {
    case 0:
      display.println("TRI");
      break; 
    case 1:
      display.println("SAW");
    break; 
    case 2:
      display.println("RMP");
    break;
     case 3:
      display.println("SQR");
    break;
    case 4:
      display.println("SIN");
    break;
  }
  
 
}




