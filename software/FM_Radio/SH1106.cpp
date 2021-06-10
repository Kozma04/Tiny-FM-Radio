#include "SH1106.h"

#define sh1106_swap(a, b) { int16_t t = a; a = b; b = t; }

inline int16_t SH1106::width() {
    return SH1106_LCDWIDTH;
}

inline int16_t SH1106::height() {
    return SH1106_LCDHEIGHT;
}

void SH1106::drawPixel(int16_t x, int16_t y, uint8_t color) {
    if((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
        return;
    drawPixelUnsafe(x, y, color);
}

void SH1106::drawPixelUnsafe(int16_t x, int16_t y, uint8_t color) {
    uint16_t pos = x + (y / 8) * SH1106_LCDWIDTH;
    static uint8_t val;
    buffer->read(pos, &val);
    switch (color) {
        case WHITE:   val |= 1 << (y & 7); break;
        case BLACK:   val &= ~(1 << (y & 7)); break;
        case INVERSE: val ^= 1 << (y & 7); break;
    }
    buffer->write(pos, val);
}

const uint8_t dif8[8] = {8, 7, 6, 5, 4, 3, 2, 1};
const uint8_t mask[8] = {0b1, 0b11, 0b111, 0b1111, 0b11111, 0b111111, 0b1111111, 0b11111111};

// takes an 8 bit portion from a 16-bit value which is shifted from the MSB to the right by rshift positions
uint8_t scroll(const uint8_t &a, const uint8_t &b, const uint8_t &rshift) {
    return (a << rshift) | (b >> dif8[rshift]);
}
// clear the last n bits from a
uint8_t clearBits(const uint8_t &a, const uint8_t &n) {
    return (a << n) >> n;//a & (~mask[n]); //(a >> n) << n;
}
// overlaps "fromlsb" bits (from MSB) from b over a. Ex.: lshift=3 -> result = aaaaabbb
uint8_t mergelsb(const uint8_t &a, const uint8_t &b, const uint8_t &fromlsb) {
    return clearBits(a, fromlsb) | (b << dif8[fromlsb]);
}

// overlaps "frommsb" bits (from MSB) from b over a. Ex.: lshift=3 -> result = aaaaabbb
uint8_t mergemsb(const uint8_t &a, const uint8_t &b, const uint8_t &frommsb) {
    return ((a >> frommsb) << frommsb) | (b >> dif8[frommsb]);
}

void ptr_swap(uint8_t **left, uint8_t **right) {
    uint8_t *swap = *left;
    *left = *right;
    *right = swap;
}

uint8_t bmpClusterA[SH1106_BMP_H_CLUSTER_SIZE], bmpClusterB[SH1106_BMP_H_CLUSTER_SIZE];
uint8_t *bmpA = bmpClusterA, *bmpB = bmpClusterB;
const uint8_t hcluster = SH1106_BMP_H_CLUSTER_SIZE;

inline constexpr bool check(const uint16_t &pos) {
    return pos >= 0 && pos < 1024;
}

void SH1106::drawBitmap(int16_t x, int16_t y, uint8_t bwidth, uint8_t bheight, Arr_Wrap<uint8_t> &bitmap, uint8_t loopWidth) {
    bheight /= 8;
    uint8_t h = 0, mod8 = y % 8, i, j, end;
    int8_t imod = 8 - mod8;
    int16_t pos = (y / 8) * SH1106_LCDWIDTH + x;
    uint8_t actualWidth = bwidth * loopWidth;
    int16_t bpos = 0;

    uint8_t midEnd = (mod8 == 0) ? (bheight) : (bheight-1);

    if(check(pos)) {
        buffer->readRange(pos, cluster, actualWidth);
        for(i = 0; i < actualWidth && i < SH1106_LCDWIDTH;) {
            end = min(bwidth, actualWidth - i);
            for(j = 0; j < end; i++, j++)
                cluster[i] = mergelsb(cluster[i], bmpA[j], imod);
        }
        buffer->writeRange(pos, cluster, actualWidth);
        h++;
    }
    if(mod8 == 0)bpos = -bwidth;
    if(mod8 > 0)pos += SH1106_LCDWIDTH;

    while(h < midEnd) {
        uint16_t nbpos = bpos + bwidth;
        bitmap.readRange(nbpos, bmpB, bwidth);
        if(check(pos)) {
            for(i = 0; i < actualWidth && i < SH1106_LCDWIDTH;) {
                end = min(bwidth, actualWidth - i);
                for(j = 0; j < end; i++, j++)
                    cluster[i] = (mod8 == 0 )? bmpB[j] : scroll(bmpB[j], bmpA[j], mod8);
                buffer->writeRange(pos, cluster, actualWidth);
            }
        }

        ptr_swap(&bmpA, &bmpB);

        pos += SH1106_LCDWIDTH, bpos = nbpos, h++;
    }

    if(mod8 > 0 && check(pos)) {
        bitmap.readRange(bwidth*(bheight-1), bmpA, bwidth);
        buffer->readRange(pos, cluster, actualWidth);
        for(i = 0; i < actualWidth && i < SH1106_LCDWIDTH;) {
            end = min(bwidth, actualWidth - i);
            for(j = 0; j < end; i++, j++)
                cluster[i] = mergemsb(0, bmpA[j], mod8);
        }
        buffer->writeRange(pos, cluster, actualWidth);
    }
}

void SH1106::begin(FramebufferAlloc fb, uint8_t vccstate, uint8_t i2caddr, bool reset) {
    if(fb == FRAMEBUFFER_INT)
        buffer = new Int_Arr<uint8_t>(SH1106_LCDHEIGHT * SH1106_LCDWIDTH / 8);

    _vccstate = vccstate;
    _i2caddr = i2caddr;

    // Init sequence for 128x64 OLED module
    command(SH1106_DISPLAYOFF);                    // 0xAE
    command(SH1106_SETDISPLAYCLOCKDIV);            // 0xD5
    command(0x80);                                  // the suggested ratio 0x80
    command(SH1106_SETMULTIPLEX);                  // 0xA8
    command(0x3F);
    command(SH1106_SETDISPLAYOFFSET);              // 0xD3
    command(0x00);                                   // no offset

    command(SH1106_SETSTARTLINE | 0x0);            // line #0 0x40
    command(SH1106_CHARGEPUMP);                    // 0x8D
    if (vccstate == SH1106_EXTERNALVCC)
      { command(0x10); }
    else
      { command(0x14); }
    command(SH1106_MEMORYMODE);                    // 0x20
    command(0x00);                                  // 0x0 act like ks0108
    command(SH1106_SEGREMAP | 0x1);
    command(SH1106_COMSCANDEC);
    command(SH1106_SETCOMPINS);                    // 0xDA
    command(0x12);
    command(SH1106_SETCONTRAST);                   // 0x81
    if (vccstate == SH1106_EXTERNALVCC) command(0x9F);
    else command(0xCF);
    command(SH1106_SETPRECHARGE);                  // 0xd9
    if (vccstate == SH1106_EXTERNALVCC) command(0x22);
    else command(0xF1);
    command(SH1106_SETVCOMDETECT);                 // 0xDB
    command(0x40);
    command(SH1106_DISPLAYALLON_RESUME);           // 0xA4
    command(SH1106_NORMALDISPLAY);                 // 0xA6

    command(SH1106_DISPLAYON);
}


void SH1106::invert(uint8_t i) {
    if (i) {
        command(SH1106_INVERTDISPLAY);
    } else {
        command(SH1106_NORMALDISPLAY);
    }
}

void SH1106::command(uint8_t c) {
    TinyWire.beginTransmission(_i2caddr);
    TinyWire.write(0); // Co = 0, D/C = 0
    TinyWire.write(c);
    TinyWire.endTransmission(true);
}

void SH1106::data(uint8_t c) {
    TinyWire.beginTransmission(_i2caddr);
    TinyWire.write(0x40); // Co = 0, D/C = 1
    TinyWire.write(c);
    TinyWire.endTransmission(true);
}

void SH1106::displayCluster(const uint8_t &size) {
    TinyWire.beginTransmission(_i2caddr);
    TinyWire.write(0x40);
    for (uint8_t l = 0; l < size; l++)
        TinyWire.write(cluster[l]);
    TinyWire.endTransmission(true);
}

void SH1106::display(void) {

    command(SH1106_SETLOWCOLUMN | 0x0);  // low col = 0
    command(SH1106_SETHIGHCOLUMN | 0x0);  // hi col = 0
    command(SH1106_SETSTARTLINE | 0x0); // line #0

	byte height=64;
	byte width=132;
	byte m_row = 0;
	byte m_col = 2;

	height >>= 3;
	width >>= 3;

	int p = 0, l;

	//uint8_t i, j, k = 0;
    //uint8_t r = width % SH1106_CLUSTER_SIZE;
    uint16_t i = 0;

	for (uint8_t page = 0; page < 8; page++) {
        command(0xB0 + page);
        command(0x02); // low column start address
        command(0x10); // high column start address

        //for(uint8_t pixel = 0; pixel < SH1106_LCDWIDTH; pixel++) {
            for (uint16_t j = 0; j < SH1106_LCDWIDTH; j+= SH1106_CLUSTER_SIZE) {
                //SRAM.read(buffer->getAddress() + p, cluster, SH1106_CLUSTER_SIZE);
                buffer->readRange(i, cluster, SH1106_CLUSTER_SIZE);
                displayCluster(SH1106_CLUSTER_SIZE);
                i += SH1106_CLUSTER_SIZE;
            }
            //p = lp;
        //}

        //p += width;
	}
}


void SH1106::clear(uint8_t val) {
    uint16_t num = SH1106_LCDWIDTH*SH1106_LCDHEIGHT/8;
    buffer->setValues(val, num);
}

void SH1106::drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
    // Do bounds/limit checks
    if(y < 0 || y >= SH1106_LCDHEIGHT) return;

    // make sure we don't try to draw below 0
    if(x < 0) {
        w += x;
        x = 0;
    }

    // make sure we don't go off the edge of the display
    if( (x + w) > SH1106_LCDWIDTH)
        w = (SH1106_LCDWIDTH - x);

      // if our width is now negative, halt
    if(w <= 0) return;

    // set up the pointer for movement through the buffer
    register uint16_t pBuf = (y / 8) * SH1106_LCDWIDTH + x;

    register uint8_t mask = 1 << (y & 7);
    uint8_t val;
    uint8_t k = SH1106_CLUSTER_SIZE, l;

    switch (color)
    {
        case WHITE:
            while(w) {
                if(w < SH1106_CLUSTER_SIZE) k = w;
                buffer->readRange(pBuf, cluster, k);
                for(l = 0; l < k; l++) cluster[l] |= mask;
                buffer->writeRange(pBuf, cluster, k);
                pBuf += k;
                w -= k;
            };
            break;
        case BLACK:
            mask = ~mask;
            while(w) {
                if(w < SH1106_CLUSTER_SIZE) k = w;
                buffer->readRange(pBuf, cluster, k);
                for(l = 0; l < k; l++) cluster[l] &= mask;
                buffer->writeRange(pBuf, cluster, k);
                pBuf += k;
                w -= k;
            };
            break;
        case INVERSE:
            while(w) {
                if(w < SH1106_CLUSTER_SIZE) k ^= w;
                buffer->readRange(pBuf, cluster, k);
                for(l = 0; l < k; l++) cluster[l] |= mask;
                buffer->writeRange(pBuf, cluster, k);
                pBuf += k;
                w -= k;
            };
            break;
    }
}


void SH1106::drawFastVLine(int16_t x, int16_t __y, int16_t __h, uint8_t color, bool usePattern) {

    // do nothing if we're off the left or right side of the screen
    if(x < 0 || x >= SH1106_LCDWIDTH) return;

    // make sure we don't try to draw below 0
    if(__y < 0) {
        // __y is negative, this will subtract enough from __h to account for __y being 0
        __h += __y;
        __y = 0;
    }

    // make sure we don't go past the height of the display
    if( (__y + __h) > SH1106_LCDHEIGHT) __h = (SH1106_LCDHEIGHT - __y);

    // if our height is now negative, punt
    if(__h <= 0) return;

    // this display doesn't need ints for coordinates, use local byte registers for faster juggling
    register uint8_t y = __y;
    register uint8_t h = __h;

    // set up the pointer for fast movement through the buffer
    //register uint8_t *pBuf = buffer;
    uint16_t pBuf = ((y / 8) * SH1106_LCDWIDTH) + x;

    // do the first partial byte, if necessary - this requires some masking
    register uint8_t mod = (y & 7);
    if(mod) {
        // mask off the high n bits we want to set
        mod = 8 - mod;

        // note - lookup table results in a nearly 10% performance improvement in fill* functions
        // register uint8_t mask = ~(0xFF >> (mod));
        static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
        register uint8_t mask = premask[mod];
        if(usePattern) mask &= color;

        // adjust the mask if we're not going to reach the end of this byte
        if(h < mod)
            mask &= (0XFF >> (mod - h));

        uint8_t val = buffer->read(pBuf);

        if(usePattern) val |= mask;
        else {
            switch (color) {
                case WHITE:   val |=  mask;  break;
                case BLACK:   val &= ~mask;  break;
                case INVERSE: val ^=  mask;  break;
            }
        }

        buffer->write(pBuf, val);

        // fast exit if we're done here!
        if(h < mod) return;

        h -= mod;

        pBuf += SH1106_LCDWIDTH;
    }


    // write solid bytes while we can - effectively doing 8 rows at a time
    if(h >= 8) {
        uint8_t val;
        if (color == INVERSE)  { // separate copy of the code so we don't impact performance of the black/white write version with an extra comparison per loop
            do  {
                val = buffer->read(pBuf);
                val=~val;
                buffer->write(pBuf, val);
                // adjust the buffer forward 8 rows worth of data
                pBuf += SH1106_LCDWIDTH;

                // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
                h -= 8;
            } while(h >= 8);
        }
        else {
            // store a local value to work with
            register uint8_t val;
            if(usePattern) val = color;
            else val = (color == WHITE) ? 255 : 0;
            do  {
                // write our value in
                buffer->write(pBuf, val);
                // adjust the buffer forward 8 rows worth of data
                pBuf += SH1106_LCDWIDTH;
                // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
                h -= 8;
            } while(h >= 8);
        }
    }

    // now do the final partial byte, if necessary
    if(h) {
        mod = h & 7;
        // this time we want to mask the low bits of the byte, vs the high bits we did above
        // register uint8_t mask = (1 << mod) - 1;
        // note - lookup table results in a nearly 10% performance improvement in fill* functions
        static uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
        register uint8_t mask = postmask[mod];
        uint8_t val = buffer->read(pBuf);
        if(usePattern) {
            mask &= color;
            val |= mask;
        }
        else {
            switch (color) {
                case WHITE:   val |=  mask;  break;
                case BLACK:   val &= ~mask;  break;
                case INVERSE: val ^=  mask;  break;
            }
        }
        buffer->write(pBuf, val);
    }
}

void SH1106::line(int16_t x, int16_t y, int16_t x2, int16_t y2, uint8_t color) {
    bool yLonger = false;
    int16_t shortLen = y2 - y;
    int16_t longLen = x2 - x;
    if(abs(shortLen) > abs(longLen)) {
        int16_t swap = shortLen;
        shortLen = longLen;
        longLen = swap;
        yLonger = true;
    }
    int16_t decInc;
    if (longLen == 0) decInc = 0;
    else decInc = (shortLen << 8) / longLen;

    if (yLonger) {
        if (longLen > 0) {
            longLen += y;
            for (int16_t j=0x80 + (x << 8); y <= longLen; ++y) {
                drawPixel(j >> 8, y, color);
                j += decInc;
            }
            return;
        }
        longLen += y;
        for (int16_t j=0x80 + (x << 8); y >= longLen; --y) {
            drawPixel(j >> 8, y, color);
            j -= decInc;
        }
        return;
    }

    if (longLen > 0) {
        longLen += x;
        for (int16_t j= 0x80 + (y << 8); x <= longLen; ++x) {
            drawPixel(x, j >> 8, color);
            j += decInc;
        }
        return;
    }
    longLen += x;
    for (int16_t j = 0x80 + (y << 8); x >= longLen; --x) {
        drawPixel(x, j >> 8, color);
        j -= decInc;
    }
}

void SH1106::drawChar(uint8_t x, uint8_t pageY, uint8_t character, uint8_t color, uint8_t background) {
    uint16_t fontPos = (character - 32) * _fontWidth;
    uint16_t pos = pageY * SH1106_LCDWIDTH + x;
    for(uint8_t i = 0; i < _fontWidth; i++, pos++) {
        //uint8_t col = pgm_read_byte(_font + fontPos + i);
        uint8_t col = _font[fontPos + i];
        uint8_t t;

        switch(background) {
            case NO_BACKGROUND: t = buffer->read(pos); break;
            case WHITE: t = 0xFF; break;
            case BLACK: t = 0; break;
            case INVERSE: t = buffer->read(pos) ^ 0xFF; break;
        }
        switch(color) {
            case WHITE: t |= col; break;
            case BLACK: t &= ~col; break;
            case INVERSE: t ^= col; break;
        }

        buffer->write(pos, t);
    }
}
void SH1106::setFont(uint8_t *font, uint8_t fontWidth, uint8_t fontHeight, uint8_t fontSpacing) {
    _font = font;
    _fontWidth = fontWidth;
    _fontHeight = fontHeight;
    _fontSpacing = fontSpacing;
}

size_t SH1106::write(uint8_t character) {
    uint8_t xAdd = _fontWidth + _fontSpacing;
    if(character == '\n') cy++;
    else if(character == '\r') cx = 0;
    else if(cx + _fontWidth + _fontSpacing >= SH1106_LCDWIDTH) cx = 0, cy++;
    else {
        cy = min(cy, 8);
        drawChar(cx, cy, character, fontColor, backColor);
        uint16_t pos = cy * SH1106_LCDWIDTH + cx+_fontWidth;
        for(int i = cx+_fontWidth; i <= cx+xAdd; i++, pos++) {
            uint8_t t;
            
            switch(backColor) {
                case NO_BACKGROUND: t = buffer->read(pos); break;
                case WHITE: t = 0xFF; break;
                case BLACK: t = 0; break;
                case INVERSE: t = buffer->read(pos) ^ 0xFF; break;
            }
            buffer->write(pos, t);
        }
        cx += xAdd;
    }

    return 1;
}

void SH1106::setCursor(uint8_t x, uint8_t yPage) {
    cx = x;
    cy = yPage;
}

SH1106 oled;
