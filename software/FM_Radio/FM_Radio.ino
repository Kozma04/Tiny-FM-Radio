#include <Wire.h>
#include "font5x7.h"
#include "tinytwi.h"
#include "SH1106.h"

#define PIN_PWR   PIN_PB2
#define PIN_BAT   PIN_PA2
#define PIN_OK    PIN_PB3
#define PIN_UP    PIN_PB5
#define PIN_DOWN  PIN_PB4
#define PIN_LEFT  PIN_PA6
#define PIN_RIGHT PIN_PA7
#define PIN_ASHDN PIN_PC1

#define PIN_PSW   PIN_PA4
#define PIN_VOLM  PIN_PC3
#define PIN_VOLP  PIN_PC0
#define PIN_SEEKM PIN_PA1
#define PIN_SEEKP PIN_PA3

enum Action {
    LEFT, RIGHT, OK
};

struct Entry {
    char name[24] = {0};
    void (*callback)(Entry*, Action) = 0;
};

struct Menu {
    char name[16] = {0};
    Entry *entries[7];
    uint8_t nEntries;
};

Menu *menu;

uint8_t hovered;
bool mute = false;
int volume;

void renderMenu() {
    oled.clear();
    oled.setCursor(0, 0);
    oled.fontColor = INVERSE;
    oled.backColor = BLACK;
    oled.println(menu->name);
    for(int i = 0; i < menu->nEntries; i++) {
        if(i == hovered) oled.backColor = WHITE;
        oled.println(menu->entries[i]->name);
        if(i == hovered) oled.backColor = BLACK;
    }
}

void menuUp() {
    if(hovered > 0) hovered--;
    else hovered = menu->nEntries - 1;
}

void menuDown() {
    if(hovered < (menu->nEntries) - 1) hovered++;
    else hovered = 0;
}

void menuInteract(Action action) {
    if(menu->entries[hovered]->callback != 0)
        menu->entries[hovered]->callback(menu->entries[hovered], action);
}

void setMenu(Menu *newMenu) {
    hovered = 0;
    menu = newMenu;
}

void press(int pin) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    delay(100);
    pinMode(pin, INPUT);
    delay(100);
}

Menu home;
Entry eBattery;
Entry eShutdown;
Entry eMute;
Entry eVolume;
Entry eSeek;
Entry ePsw;

uint32_t t;

void shutdown() {
    oled.command(SH1106_DISPLAYOFF);
    delay(100);
    PORTB.OUTCLR = 1 << 2;
    for(;;);
}

void shutdownCallback(Entry *entry, Action action) {
    if((PORTB.IN & (1 << 3)) == 0) {
        t = millis();
        while((PORTB.IN & (1 << 3)) == 0) {
            if(millis() - t > 1500) {
                shutdown();
            }
        }
    }
}

void muteCallback(Entry *entry, Action action) {
    mute = !mute;
    if(mute) {
        strcpy(eMute.name, " Mute: DA");
        digitalWrite(PIN_ASHDN, LOW);
    }
    else {
        strcpy(eMute.name, " Mute: NU");
        digitalWrite(PIN_ASHDN, HIGH);
    }
}

void volumeCallback(Entry *entry, Action action) {
    if(action == LEFT) {
        press(PIN_VOLM);
    }
    else if(action == RIGHT) {
        press(PIN_VOLP);
    }
}

void seekCallback(Entry *entry, Action action) {
    if(action == LEFT) {
        press(PIN_SEEKM);
    }
    else if(action == RIGHT) {
        press(PIN_SEEKP);
    }
}

void pswCallback(Entry *entry, Action action) {
    press(PIN_PSW);
}

void initMenu() {
    setMenu(&home);
    strcpy(home.name, "Radio FM");
    home.nEntries = 6;
    home.entries[0] = &eBattery;
    home.entries[1] = &eMute;
    home.entries[2] = &eVolume;
    home.entries[3] = &eSeek;
    home.entries[4] = &ePsw;
    home.entries[5] = &eShutdown;

    strcpy(eBattery.name, " [no battery data]");
    strcpy(eMute.name, " Mute: NU");
    strcpy(eVolume.name, " Volum [- / +]");
    strcpy(eShutdown.name, " Oprire");
    strcpy(eSeek.name, " Canal [- / +]");
    strcpy(ePsw.name, " PSW (buton radio)");
    eShutdown.callback = shutdownCallback;
    eMute.callback = muteCallback;
    eVolume.callback = volumeCallback;
    eSeek.callback = seekCallback;
    ePsw.callback = pswCallback;
}

int appendInt(char *c, int t, int pos) {
    int p10 = pow(10, getNc(t) - 1);
    while(p10 > 0) {
        c[pos++] = '0' + (t / p10 % 10);
        p10 /= 10;
    }
    return pos;
}

void setup() {
    pinMode(PIN_ASHDN, OUTPUT);
    digitalWrite(PIN_ASHDN, LOW);
    PORTB.DIRCLR = 1 << 2;
    PORTB.DIRCLR = 1 << 5; // UP
    PORTB.PIN5CTRL = 1 << 3;
    uint32_t t = millis();
    bool ok = true;
    while(millis() - t < 1000) {
        if((PORTB.IN & (1 << 5)) != 0) {
            ok = false;
        }
    }
    if(!ok) {
        PORTB.DIRSET = 1 << 2;
        PORTB.OUTCLR = 1 << 2;
        for(;;);
    }

    PORTB.DIRSET = 1 << 2;
    PORTB.OUTSET = 1 << 2;
    PORTB.OUTCLR = 1 << 2;
    PORTB.OUTSET = 1 << 2;

    PORTB.DIRCLR = 1 << 3;
    PORTB.PIN3CTRL = 1 << 3;

    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
    analogReadResolution(10);

    TinyWire.begin();
    TinyWire.setSpeed(FAST_MODE);
    oled.setFont(font5x7, 5, 8, 1);

    oled.begin(FRAMEBUFFER_INT);

    oled.clear();
    oled.setCursor(0, 0);
    oled.println("Initializare...");
    oled.display();

    delay(500);
    oled.command(SH1106_DISPLAYOFF);
    delay(500);
    oled.command(SH1106_DISPLAYON);

    // set display orientation
    oled.command(0xc0 | 0x08);
    oled.command(0xa0 | 0x01);

    initMenu();

    pinMode(PIN_ASHDN, OUTPUT);
    digitalWrite(PIN_ASHDN, HIGH);
    for(int i = 0; i < 20; i++) press(PIN_VOLM);
}

uint8_t getNc(uint8_t x) {
    if(x >= 100) return 3;
    if(x >= 10) return 2;
    return 1;
}

uint32_t lastPress;
uint32_t f;

float bat = 1000;
float lowThreshold = 3.15; // Battery voltage considered low
float criticalThreshold = 3; // shutdown voltage

void loop() {
    f++;
    if(f % 10 == 0) {
        uint32_t val = 0;
        for(int i = 0; i < 10; i++)
            val += analogRead(PIN_BAT);
        val /= 10;
        bat = (val / 1024.f * 3.3f) * 6900.f / 4700.f;
        int p = min(100, max(0, int((bat - 3.f) / (4.2f - 3.f) * 100.f)));
        memset(eBattery.name, 0, 24);
        strcat(eBattery.name, " Bat.: ");
        int pos = 7;
        pos = appendInt(eBattery.name, int(bat), pos);
        eBattery.name[pos++] = '.';
        pos = appendInt(eBattery.name, int(bat * 100) % 100, pos);
        eBattery.name[pos++] = 'v';
        eBattery.name[pos++] = ' ';
        eBattery.name[pos++] = '/';
        eBattery.name[pos++] = ' ';
        pos = appendInt(eBattery.name, p, pos);
        eBattery.name[pos++] = '%';
    }

    /*
    oled.clear();
    oled.setCursor(0, 0);
    oled.fontColor = INVERSE;
    oled.backColor = WHITE;
    oled.print(" Readings:");
    oled.backColor = BLACK;
    oled.println(f);

    oled.print("Battery: ");
    oled.print(bat);
    oled.print("v / ");
    oled.print(int((bat - 3.f) / (4.2f - 3.f) * 100.f));
    oled.println("%");

    if(digitalRead(PIN_UP) == LOW) {
        press(PIN_VOLP);
        oled.println("Vol up");
    }
    else if(digitalRead(PIN_DOWN) == LOW) {
        press(PIN_VOLM);
        oled.println("Vol down");
    }
    else if(digitalRead(PIN_LEFT) == LOW) {
        press(PIN_SEEKM);
        oled.println("Seek -");
    }
    else if(digitalRead(PIN_RIGHT) == LOW) {
        press(PIN_SEEKP);
        oled.println("Seek +");
    }

    oled.display();
    delay(1000);*/

    if(millis() - lastPress >= 300) {
        bool pressed = false;
        if(digitalRead(PIN_UP) == LOW) {
            menuUp();
            pressed = true;
        }
        else if(digitalRead(PIN_DOWN) == LOW) {
            menuDown();
            pressed = true;
        }
        else if(digitalRead(PIN_LEFT) == LOW) {
            menuInteract(LEFT);
            pressed = true;
        }
        else if(digitalRead(PIN_RIGHT) == LOW) {
            menuInteract(RIGHT);
            pressed = true;
        }
        else if(digitalRead(PIN_OK) == LOW) {
            menuInteract(OK);
            pressed = true;
        }
        if(pressed) lastPress = millis();
    }

    renderMenu();
    if(bat < lowThreshold) {
        if(f % 80 > 40) {
            oled.invert(true);
            oled.backColor = WHITE;
        }
        else {
            oled.invert(false);
            oled.backColor = WHITE;
        }
        oled.print("BATERIE DESCARCATA"); // low battery
        oled.backColor = BLACK;
    }
    else oled.invert(false);
    if(bat < criticalThreshold) shutdown();
    oled.display();
}
