#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

class Graphics {
public:
    static void initialize();
    static void set_mode_text();
    static void set_mode_graphics();
    static void put_pixel(int x, int y, uint8_t color);
    static uint8_t get_pixel(int x, int y);
    static void draw_rect(int x, int y, int width, int height, uint8_t color);
    static void clear_screen(uint8_t color);
    static void set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    static void draw_char(int x, int y, char c, uint8_t color);
    static void draw_char_small(int x, int y, char c, uint8_t color);
    static void draw_text(int x, int y, const char* text, uint8_t color);
    static void draw_text_small(int x, int y, const char* text, uint8_t color);
    static void draw_image(int x, int y, int width, int height, const uint8_t* data);

private:
    static uint8_t* video_memory;
    static bool is_graphics_mode;
};

#endif
