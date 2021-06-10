// Modified code to alloved clustered memory access. Originally intended for compatibility with
// using an external SRAM IC via I2C for storing the framebuffer. This library uses a temporary buffer
// in conjunction with memhelper in order to reduce I2C interface transaction begin/end overhead.

// Base library:
// https://github.com/wonho-maker/Adafruit_SH1106

#ifndef SH1106_H
#define SH1106_H

#include <avr/pgmspace.h>
#include <Arduino.h>
#include "tinytwi.h"
#include "memhelper.h"

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define NO_BACKGROUND 3 // for text drawing ONLY

#define SH1106_I2C_ADDRESS   0x3C

#define SH1106_EXT_BUFFER // Allocates the display framebuffer on the 47C16 I2C external SRAM. USE ONLY IF IC IS AVAILABLE
#define SH1106_INT_BUFFER

#define SH1106_EXT_BUFFER_ADDR 1024 // allocate the framebuffer at a certain address on the 47C16 if applicable

// How many bytes to extract from the framebuffer before sending them to the display
// WARNING: Must be dividable by and <= width, and smaller than the max Wire buffer length
#define SH1106_CLUSTER_SIZE 64
#define SH1106_BMP_H_CLUSTER_SIZE 64

#define SH1106_LCDWIDTH  128
#define SH1106_LCDHEIGHT 64

#define SH1106_SETCONTRAST 0x81
#define SH1106_DISPLAYALLON_RESUME 0xA4
#define SH1106_DISPLAYALLON 0xA5
#define SH1106_NORMALDISPLAY 0xA6
#define SH1106_INVERTDISPLAY 0xA7
#define SH1106_DISPLAYOFF 0xAE
#define SH1106_DISPLAYON 0xAF

#define SH1106_SETDISPLAYOFFSET 0xD3
#define SH1106_SETCOMPINS 0xDA

#define SH1106_SETVCOMDETECT 0xDB

#define SH1106_SETDISPLAYCLOCKDIV 0xD5
#define SH1106_SETPRECHARGE 0xD9

#define SH1106_SETMULTIPLEX 0xA8

#define SH1106_SETLOWCOLUMN 0x00
#define SH1106_SETHIGHCOLUMN 0x10

#define SH1106_SETSTARTLINE 0x40

#define SH1106_MEMORYMODE 0x20
#define SH1106_COLUMNADDR 0x21
#define SH1106_PAGEADDR   0x22

#define SH1106_COMSCANINC 0xC0
#define SH1106_COMSCANDEC 0xC8

#define SH1106_SEGREMAP 0xA0

#define SH1106_CHARGEPUMP 0x8D

#define SH1106_EXTERNALVCC 0x1
#define SH1106_SWITCHCAPVCC 0x2

enum FramebufferAlloc {
    FRAMEBUFFER_INT,
    FRAMEBUFFER_EXT,
    NONE
};

class SH1106 : public Print {
public:
    Arr_Wrap<uint8_t> *buffer;

    uint8_t cx = 0, cy = 0;
    uint8_t fontColor = WHITE;
    uint8_t backColor = BLACK;

    void begin(FramebufferAlloc fb, uint8_t switchvcc = SH1106_SWITCHCAPVCC, uint8_t i2caddr = SH1106_I2C_ADDRESS, bool reset=true);
    void command(uint8_t c);
    void data(uint8_t c);

    void clear(uint8_t val = 0);
    void invert(uint8_t i);
    void display();

    void drawPixel(int16_t x, int16_t y, uint8_t color);
    void drawPixelUnsafe(int16_t x, int16_t y, uint8_t color);

    void line(int16_t x, int16_t y, int16_t x2, int16_t y2, uint8_t color);

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color, bool usePattern = false);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color);

    void drawBitmap(int16_t x, int16_t y, uint8_t bwidth, uint8_t bheight, Arr_Wrap<uint8_t> &bitmap, uint8_t loopWidth = 1);

    void drawChar(uint8_t x, uint8_t pageY, uint8_t character, uint8_t color, uint8_t background);
    void setFont(uint8_t *font, uint8_t fontWidth, uint8_t fontHeight, uint8_t fontSpacing);

    size_t write(uint8_t character);
    void setCursor(uint8_t x, uint8_t yPage);

    int16_t width();
    int16_t height();

private:
    int8_t _i2caddr, _vccstate;
    uint8_t *_font;
    uint8_t _fontWidth, _fontHeight, _fontSpacing;

    uint8_t cluster[SH1106_CLUSTER_SIZE];
    void displayCluster(const uint8_t &size);
};

extern SH1106 oled;

#endif
