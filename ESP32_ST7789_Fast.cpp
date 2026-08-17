// Fast ST7789 IPS 240x240 SPI display library
// (c) 2019-20 by Pawel A. Hernik

#include "ESP32_ST7789_Fast.h"
#include <SPI.h>

// -----------------------------------------
// ST7789 commands

#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01

#define ST7789_SLPIN   0x10  // sleep on
#define ST7789_SLPOUT  0x11  // sleep off
#define ST7789_PTLON   0x12  // partial on
#define ST7789_NORON   0x13  // partial off
#define ST7789_INVOFF  0x20  // invert off
#define ST7789_INVON   0x21  // invert on
#define ST7789_DISPOFF 0x28  // display off
#define ST7789_DISPON  0x29  // display on
#define ST7789_IDMOFF  0x38  // idle off
#define ST7789_IDMON   0x39  // idle on

#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_PTLAR    0x30   // partial start/end
#define ST7789_VSCRDEF  0x33   // SETSCROLLAREA
#define ST7789_VSCRSADD 0x37

#define ST7789_WRDISBV  0x51
#define ST7789_WRCTRLD  0x53
#define ST7789_WRCACE   0x55
#define ST7789_WRCABCMB 0x5e

#define ST7789_POWSAVE    0xbc
#define ST7789_DLPOFFSAVE 0xbd

// bits in MADCTL
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00

#define ST7789_240x240_XSTART 0
#define ST7789_240x240_YSTART 0

#define ST_CMD_DELAY   0x80

// Initialization commands for ST7789 240x240 1.3" IPS
// taken from Adafruit
static const uint8_t PROGMEM init_240x240[] = {
    9,                       				// 9 commands in list:
    ST7789_SWRESET,   ST_CMD_DELAY,  		// 1: Software reset, no args, w/delay
      150,                     				// 150 ms delay
    ST7789_SLPOUT ,   ST_CMD_DELAY,  		// 2: Out of sleep mode, no args, w/delay
      255,                    				// 255 = 500 ms delay
    ST7789_COLMOD , 1+ST_CMD_DELAY,  		// 3: Set color mode, 1 arg + delay:
      0x55,                   				// 16-bit color
      10,                     				// 10 ms delay
    ST7789_MADCTL , 1,  					// 4: Memory access ctrl (directions), 1 arg:
      0x00,                   				// Row addr/col addr, bottom to top refresh
    ST7789_CASET  , 4,  					// 5: Column addr set, 4 args, no delay:
      0x00, ST7789_240x240_XSTART,          // XSTART = 0
	    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) >> 8,
	    (ST7789_TFTWIDTH+ST7789_240x240_XSTART) & 0xFF,   // XEND = 240
    ST7789_RASET  , 4,  					// 6: Row addr set, 4 args, no delay:
      0x00, ST7789_240x240_YSTART,          // YSTART = 0
      (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) >> 8,
	    (ST7789_TFTHEIGHT+ST7789_240x240_YSTART) & 0xFF,	// YEND = 240
    ST7789_INVON ,   ST_CMD_DELAY,  		// 7: Inversion ON
      10,
    ST7789_NORON  ,   ST_CMD_DELAY,  		// 8: Normal display on, no args, w/delay
      10,                     				// 10 ms delay
    ST7789_DISPON ,   ST_CMD_DELAY,  		// 9: Main screen turn on, no args, w/delay
      20
};
// -----------------------------------------

static SPISettings spiSettings(27000000, MSBFIRST, SPI_MODE3);

#define SPI_START SPI.beginTransaction(spiSettings)
#define SPI_END   SPI.endTransaction()
#define DC_DATA     digitalWrite(dcPin, HIGH)
#define DC_COMMAND  digitalWrite(dcPin, LOW)
#ifndef CS_ALWAYS_LOW
#define CS_IDLE     do { if (csPin >= 0) digitalWrite(csPin, HIGH); } while (0)
#define CS_ACTIVE   do { if (csPin >= 0) digitalWrite(csPin, LOW); } while (0)
#else
#define CS_IDLE
#define CS_ACTIVE
#endif

// ESP32 SPI helpers. The AVR SPDR/inline-assembly path is replaced by the
// ESP32 Arduino SPI API while retaining the original RGB565 byte order.
inline void ESP32_ST7789_Fast::writeSPI(uint8_t c) { SPI.transfer(c); }

inline void ESP32_ST7789_Fast::writeMulti(uint16_t color, uint16_t num) {
  while (num--) SPI.transfer16(color);
}

inline void ESP32_ST7789_Fast::copyMulti(uint8_t *img, uint16_t num) {
  while (num--) {
    SPI.transfer(*img++);
    SPI.transfer(*img++);
  }
}

ESP32_ST7789_Fast::ESP32_ST7789_Fast(int8_t dc, int8_t rst, int8_t cs, int8_t sclk, int8_t mosi) : Adafruit_GFX(ST7789_TFTWIDTH, ST7789_TFTHEIGHT)
{
  csPin = cs;
  dcPin = dc;
  rstPin = rst;
  sclkPin = sclk;
  mosiPin = mosi;
}

void ESP32_ST7789_Fast::setSPIPins(int8_t sclk, int8_t mosi) {
  sclkPin = sclk;
  mosiPin = mosi;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::init(uint16_t width, uint16_t height) 
{
  commonST7789Init(NULL);

  if(width==240 && height==240) {
    _colstart = 0;
    _rowstart = 80;
  } else {
    _colstart = 0;
    _rowstart = 0;
  }
  _ystart = _xstart = 0;
  _width  = width;
  _height = height;

  displayInit(init_240x240);
  setRotation(2);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::writeCmd(uint8_t c) 
{
  DC_COMMAND;
  CS_ACTIVE;
  SPI_START;

  writeSPI(c);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::writeData(uint8_t d8) 
{
  DC_DATA;
  CS_ACTIVE;
  SPI_START;
    
  writeSPI(d8);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::writeData16(uint16_t d16) 
{
  DC_DATA;
  CS_ACTIVE;
  SPI_START;
    
  writeMulti(d16,1);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::displayInit(const uint8_t *addr) 
{
  uint8_t  numCommands, numArgs;
  uint16_t ms;
  numCommands = pgm_read_byte(addr++);
  while(numCommands--) {
    writeCmd(pgm_read_byte(addr++));
    numArgs  = pgm_read_byte(addr++);
    ms       = numArgs & ST_CMD_DELAY;
    numArgs &= ~ST_CMD_DELAY;
    while(numArgs--) writeData(pgm_read_byte(addr++));

    if(ms) {
      ms = pgm_read_byte(addr++);
      if(ms==255) ms=500;
      delay(ms);
    }
  }
}

// ----------------------------------------------------------
// Initialization code common to all ST7789 displays
void ESP32_ST7789_Fast::commonST7789Init(const uint8_t *cmdList)
{
  pinMode(dcPin, OUTPUT);
#ifndef CS_ALWAYS_LOW
  if (csPin >= 0) {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
  }
#endif

  SPI.begin(sclkPin, -1, mosiPin, csPin);

  if (rstPin != -1) {
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, HIGH);
    delay(50);
    digitalWrite(rstPin, LOW);
    delay(50);
    digitalWrite(rstPin, HIGH);
    delay(50);
  }

  if(cmdList) displayInit(cmdList);
}

void ESP32_ST7789_Fast::beginWrite() { SPI_START; CS_ACTIVE; }
void ESP32_ST7789_Fast::endWrite() { CS_IDLE; SPI_END; }

// ----------------------------------------------------------
void ESP32_ST7789_Fast::setRotation(uint8_t m) 
{
  writeCmd(ST7789_MADCTL);
  rotation = m & 3;
  switch (rotation) {
   case 0:
     writeData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
     _xstart = _colstart;
     _ystart = _rowstart;
     break;
   case 1:
     writeData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
     _ystart = _colstart;
     _xstart = _rowstart;
     break;
  case 2:
     writeData(ST7789_MADCTL_RGB);
     _xstart = 0;
     _ystart = 0;
     break;
   case 3:
     writeData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
     _xstart = 0;
     _ystart = 0;
     break;
  }
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
  xs+=_xstart; xe+=_xstart;
  ys+=_ystart; ye+=_ystart;

  CS_ACTIVE;
  SPI_START;
  
  DC_COMMAND; writeSPI(ST7789_CASET);
  DC_DATA;
  writeSPI(xs >> 8); writeSPI(xs);
  writeSPI(xe >> 8); writeSPI(xe);

  DC_COMMAND; writeSPI(ST7789_RASET);
  DC_DATA;
  writeSPI(ys >> 8); writeSPI(ys);
  writeSPI(ye >> 8); writeSPI(ye);

  DC_COMMAND; writeSPI(ST7789_RAMWR);

  DC_DATA;
  // no CS_IDLE + SPI_END, DC_DATA to save memory
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::pushColor(uint16_t color) 
{
  SPI_START;
  //DC_DATA;
  CS_ACTIVE;

  writeSPI(color>>8); writeSPI(color);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::drawPixel(int16_t x, int16_t y, uint16_t color) 
{
  if(x<0 ||x>=_width || y<0 || y>=_height) return;
  setAddrWindow(x,y,x+1,y+1);

  writeSPI(color>>8); writeSPI(color);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) 
{
  if(x>=_width || y>=_height || h<=0) return;
  if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x, y+h-1);

  writeMulti(color,h);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::drawFastHLine(int16_t x, int16_t y, int16_t w,  uint16_t color) 
{
  if(x>=_width || y>=_height || w<=0) return;
  if(x+w-1>=_width)  w=_width-x;
  setAddrWindow(x, y, x+w-1, y);

  writeMulti(color,w);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::fillScreen(uint16_t color) 
{
  fillRect(0, 0,  _width, _height, color);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) 
{
  if(x>=_width || y>=_height || w<=0 || h<=0) return;
  if(x+w-1>=_width)  w=_width -x;
  if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x+w-1, y+h-1);

  writeMulti(color,w*h);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// draws image from RAM
void ESP32_ST7789_Fast::drawImage(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *img16) 
{
  // all protections should be on the application side
  if(w<=0 || h<=0) return;  // left for compatibility
  //if(x>=_width || y>=_height || w<=0 || h<=0) return;
  //if(x+w-1>=_width)  w=_width -x;
  //if(y+h-1>=_height) h=_height-y;
  setAddrWindow(x, y, x+w-1, y+h-1);

  uint32_t num = (uint32_t)w * h;
  while (num--) SPI.transfer16(*img16++);

  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// draws image from flash (PROGMEM)
void ESP32_ST7789_Fast::drawImageF(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *img16) 
{
  if(x>=_width || y>=_height || w<=0 || h<=0) return;
  setAddrWindow(x, y, x+w-1, y+h-1);
  uint32_t num = (uint32_t)w * h;
  while (num--) SPI.transfer16(pgm_read_word(img16++));
  CS_IDLE;
  SPI_END;
}

// ----------------------------------------------------------
// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ESP32_ST7789_Fast::Color565(uint8_t r, uint8_t g, uint8_t b) 
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::invertDisplay(boolean mode) 
{
  writeCmd(!mode ? ST7789_INVON : ST7789_INVOFF);  // modes inverted?
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::partialDisplay(boolean mode) 
{
  writeCmd(mode ? ST7789_PTLON : ST7789_NORON);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::sleepDisplay(boolean mode) 
{
  writeCmd(mode ? ST7789_SLPIN : ST7789_SLPOUT);
  delay(5);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::enableDisplay(boolean mode) 
{
  writeCmd(mode ? ST7789_DISPON : ST7789_DISPOFF);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::idleDisplay(boolean mode) 
{
  writeCmd(mode ? ST7789_IDMON : ST7789_IDMOFF);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::resetDisplay() 
{
  writeCmd(ST7789_SWRESET);
  delay(5);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::setScrollArea(uint16_t tfa, uint16_t bfa) 
{
  uint16_t vsa = 320-tfa-bfa; // ST7789 320x240 VRAM
  writeCmd(ST7789_VSCRDEF);
  writeData16(tfa);
  writeData16(vsa);
  writeData16(bfa);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::setScroll(uint16_t vsp) 
{
  writeCmd(ST7789_VSCRSADD);
  writeData16(vsp);
}

// ----------------------------------------------------------
void ESP32_ST7789_Fast::setPartArea(uint16_t sr, uint16_t er) 
{
  writeCmd(ST7789_PTLAR);
  writeData16(sr);
  writeData16(er);
}

// ----------------------------------------------------------
// doesn't work
void ESP32_ST7789_Fast::setBrightness(uint8_t br) 
{
  //writeCmd(ST7789_WRCACE);
  //writeData(0xb1);  // 80,90,b0, or 00,01,02,03
  //writeCmd(ST7789_WRCABCMB);
  //writeData(120);

  //BCTRL=0x20, dd=0x08, bl=0x04
  int val = 0x04;
  writeCmd(ST7789_WRCTRLD);
  writeData(val);
  writeCmd(ST7789_WRDISBV);
  writeData(br);
}

// ----------------------------------------------------------
// 0 - off
// 1 - idle
// 2 - normal
// 4 - display off
void ESP32_ST7789_Fast::powerSave(uint8_t mode) 
{
  if(mode==0) {
    writeCmd(ST7789_POWSAVE);
    writeData(0xec|3);
    writeCmd(ST7789_DLPOFFSAVE);
    writeData(0xff);
    return;
  }
  int is = (mode&1) ? 0 : 1;
  int ns = (mode&2) ? 0 : 2;
  writeCmd(ST7789_POWSAVE);
  writeData(0xec|ns|is);
  if(mode&4) {
    writeCmd(ST7789_DLPOFFSAVE);
    writeData(0xfe);
  }
}

// ------------------------------------------------
// Input a value 0 to 511 (85*6) to get a color value.
// The colours are a transition R - Y - G - C - B - M - R.
void ESP32_ST7789_Fast::rgbWheel(int idx, uint8_t *_r, uint8_t *_g, uint8_t *_b)
{
  idx &= 0x1ff;
  if(idx < 85) { // R->Y  
    *_r = 255; *_g = idx * 3; *_b = 0;
    return;
  } else if(idx < 85*2) { // Y->G
    idx -= 85*1;
    *_r = 255 - idx * 3; *_g = 255; *_b = 0;
    return;
  } else if(idx < 85*3) { // G->C
    idx -= 85*2;
    *_r = 0; *_g = 255; *_b = idx * 3;
    return;  
  } else if(idx < 85*4) { // C->B
    idx -= 85*3;
    *_r = 0; *_g = 255 - idx * 3; *_b = 255;
    return;    
  } else if(idx < 85*5) { // B->M
    idx -= 85*4;
    *_r = idx * 3; *_g = 0; *_b = 255;
    return;    
  } else { // M->R
    idx -= 85*5;
    *_r = 255; *_g = 0; *_b = 255 - idx * 3;
   return;
  }
} 

uint16_t ESP32_ST7789_Fast::rgbWheel(int idx)
{
  uint8_t r,g,b;
  rgbWheel(idx, &r,&g,&b);
  return RGBto565(r,g,b);
}

// ------------------------------------------------