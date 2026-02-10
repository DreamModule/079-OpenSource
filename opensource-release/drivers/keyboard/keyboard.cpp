#include "keyboard.h"
#include "io.h"


static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

void Keyboard::initialize() {
    
    
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        inb(KEYBOARD_DATA_PORT);  
    }
}

bool Keyboard::has_key() {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    
    
    return (status & 0x01) && !(status & 0x20);
}

uint8_t Keyboard::get_scancode() {
    
    
    for (int i = 0; i < 1000000; i++) {
        uint8_t status = inb(KEYBOARD_STATUS_PORT);

        
        if (!(status & 0x01)) {
            
            continue;
        }

        
        if (status & 0x20) {
            
            
            
            inb(KEYBOARD_DATA_PORT);
            continue;
        }

        
        return inb(KEYBOARD_DATA_PORT);
    }
    return 0;  
}

uint8_t Keyboard::get_scancode_timeout(int timeout) {
    
    for (int i = 0; i < timeout; i++) {
        uint8_t status = inb(KEYBOARD_STATUS_PORT);

        
        if (!(status & 0x01)) {
            continue;
        }

        
        if (status & 0x20) {
            
            inb(KEYBOARD_DATA_PORT);
            continue;
        }

        
        return inb(KEYBOARD_DATA_PORT);
    }
    return 0;  
}

char Keyboard::scancode_to_char(uint8_t scancode) {
    
    if (scancode & 0x80) {
        return 0;
    }

    
    if (scancode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[scancode];
    }
    return 0;
}

char Keyboard::getchar() {
    while (true) {
        uint8_t scancode = get_scancode();
        if (scancode == 0) continue;  

        char c = scancode_to_char(scancode);
        if (c != 0) {
            return c;
        }
    }
}
