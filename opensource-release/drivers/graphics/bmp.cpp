#include "bmp.h"
#include "graphics.h"
#include "string.h"

BMPHeader BMP::header;
BMPInfoHeader BMP::info;
uint8_t* BMP::image_data = nullptr;
RGBQuad* BMP::palette = nullptr;
bool BMP::loaded = false;

bool BMP::load_from_memory(const uint8_t* data, uint32_t size) {
    if (size < sizeof(BMPHeader) + sizeof(BMPInfoHeader)) {
        return false;
    }

    
    memcpy(&header, data, sizeof(BMPHeader));

    
    if (header.signature != 0x4D42) {  
        return false;
    }

    
    memcpy(&info, data + sizeof(BMPHeader), sizeof(BMPInfoHeader));

    
    if (info.bits_per_pixel != 8) {
        return false;
    }

    
    uint32_t palette_size = 256;
    if (info.colors_used > 0 && info.colors_used < 256) {
        palette_size = info.colors_used;
    }

    
    const uint8_t* palette_data = data + sizeof(BMPHeader) + info.header_size;

    
    for (uint32_t i = 0; i < palette_size; i++) {
        RGBQuad color;
        memcpy(&color, palette_data + i * sizeof(RGBQuad), sizeof(RGBQuad));
        Graphics::set_palette(i, color.red, color.green, color.blue);
    }

    
    image_data = (uint8_t*)(data + header.data_offset);
    loaded = true;

    return true;
}

void BMP::draw(int x, int y) {
    if (!loaded || !image_data) {
        return;
    }

    int width = info.width;
    int height = info.height;

    
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            
            int bmp_row = height - 1 - row;

            
            int row_size = ((width + 3) / 4) * 4;
            uint8_t pixel = image_data[bmp_row * row_size + col];

            Graphics::put_pixel(x + col, y + row, pixel);
        }
    }
}

void BMP::free() {
    image_data = nullptr;
    palette = nullptr;
    loaded = false;
}
