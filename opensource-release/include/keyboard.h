#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"


#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64


#define KEY_ESC 0x01
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ENTER 0x1C
#define KEY_X 0x2D
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_HOME 0x47
#define KEY_UP 0x48
#define KEY_PGUP 0x49
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_END 0x4F
#define KEY_DOWN 0x50
#define KEY_PGDN 0x51
#define KEY_DELETE 0x53

class Keyboard {
public:
    static void initialize();
    static char getchar();
    static uint8_t get_scancode();
    static uint8_t get_scancode_timeout(int timeout);  
    static bool has_key();
    static char scancode_to_char(uint8_t scancode);  
};

#endif
