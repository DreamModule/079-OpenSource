#include "vga.h"
#include "io.h"


uint16_t* VGA::buffer = (uint16_t*)0xB8000;
uint8_t VGA::cursor_x = 0;
uint8_t VGA::cursor_y = 0;
uint8_t VGA::color = 0;

void VGA::initialize() {
    color = make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    clear();
}

void VGA::clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            buffer[index] = make_vga_entry(' ', color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void VGA::putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else {
        const int index = cursor_y * VGA_WIDTH + cursor_x;
        buffer[index] = make_vga_entry(c, color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_cursor();
}

void VGA::write(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

void VGA::write_line(const char* str) {
    write(str);
    putchar('\n');
}

void VGA::set_color(uint8_t fg, uint8_t bg) {
    color = make_color(fg, bg);
}

void VGA::set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
    update_cursor();
}

void VGA::scroll() {
    
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
        }
    }

    
    for (int x = 0; x < VGA_WIDTH; x++) {
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_vga_entry(' ', color);
    }

    cursor_y = VGA_HEIGHT - 1;
}

void VGA::update_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

uint8_t VGA::make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

uint16_t VGA::make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}
