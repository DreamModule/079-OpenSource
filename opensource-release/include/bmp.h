#ifndef BMP_H
#define BMP_H

#include "types.h"


#pragma pack(push, 1)
struct BMPHeader {
    uint16_t signature;      
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t data_offset;
};

struct BMPInfoHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
};

struct RGBQuad {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
};
#pragma pack(pop)

class BMP {
public:
    static bool load_from_memory(const uint8_t* data, uint32_t size);
    static void draw(int x, int y);
    static void free();

private:
    static BMPHeader header;
    static BMPInfoHeader info;
    static uint8_t* image_data;
    static RGBQuad* palette;
    static bool loaded;
};

#endif
