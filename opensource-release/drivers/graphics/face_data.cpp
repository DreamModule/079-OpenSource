


#include "face_data.h"


void generate_face_data(uint8_t* buffer) {
    
    for (int i = 0; i < FACE_WIDTH * FACE_HEIGHT; i++) {
        buffer[i] = 0;
    }

    
    for (int y = 0; y < FACE_HEIGHT; y++) {
        for (int x = 0; x < FACE_WIDTH; x++) {
            int idx = y * FACE_WIDTH + x;

            float nx = (float)(x - FACE_WIDTH/2) / (FACE_WIDTH/2.2f);
            float ny = (float)(y - FACE_HEIGHT/2) / (FACE_HEIGHT/2.2f);
            float dist = (nx * nx) + (ny * ny);

            
            if (dist < 1.0f) {
                
                if ((x * 7 + y * 13) % 4 < 2) {
                    buffer[idx] = 15;  
                }
            }
        }
    }

    
    int eye_y = FACE_HEIGHT / 3;
    int eye1_x = FACE_WIDTH / 3 - 5;
    int eye2_x = 2 * FACE_WIDTH / 3 + 5;

    
    for (int y = -18; y <= 18; y++) {
        for (int x = -15; x <= 15; x++) {
            if (x*x/225 + y*y/324 < 1) {
                int idx = (eye_y + y) * FACE_WIDTH + (eye1_x + x);
                if (idx >= 0 && idx < FACE_WIDTH * FACE_HEIGHT) {
                    buffer[idx] = 15;  
                }
            }
        }
    }

    
    for (int y = -18; y <= 18; y++) {
        for (int x = -15; x <= 15; x++) {
            if (x*x/225 + y*y/324 < 1) {
                int idx = (eye_y + y) * FACE_WIDTH + (eye2_x + x);
                if (idx >= 0 && idx < FACE_WIDTH * FACE_HEIGHT) {
                    buffer[idx] = 15;  
                }
            }
        }
    }

    
    for (int x = 0; x < FACE_WIDTH; x += 5) {
        for (int y = 0; y < FACE_HEIGHT; y++) {
            if ((y + x) % 3 == 0) {
                int idx = y * FACE_WIDTH + x;
                buffer[idx] = 15;  
            }
        }
    }

    
    for (int y = 0; y < FACE_HEIGHT; y += 2) {
        for (int x = 0; x < FACE_WIDTH; x++) {
            int idx = y * FACE_WIDTH + x;
            if (buffer[idx] == 15 && (x % 3) == 0) {
                buffer[idx] = 0;  
            }
        }
    }

    
    for (int i = 0; i < 60; i++) {
        int glitch_x = (i * 67) % FACE_WIDTH;
        int glitch_y = (i * 43) % FACE_HEIGHT;
        int block_w = 8 + (i % 15);
        int block_h = 5 + (i % 10);

        uint8_t color = (i % 2) ? 15 : 0;

        for (int dy = 0; dy < block_h; dy++) {
            for (int dx = 0; dx < block_w; dx++) {
                int px = (glitch_x + dx) % FACE_WIDTH;
                int py = (glitch_y + dy) % FACE_HEIGHT;
                int idx = py * FACE_WIDTH + px;
                buffer[idx] = color;
            }
        }
    }

    
    for (int i = 0; i < FACE_WIDTH * FACE_HEIGHT / 8; i++) {
        int idx = (i * 113) % (FACE_WIDTH * FACE_HEIGHT);
        buffer[idx] = (i % 2) ? 15 : 0;
    }

    
    for (int i = 0; i < 40; i++) {
        int start_x = (i * 13) % FACE_WIDTH;
        int start_y = (i * 17) % FACE_HEIGHT;

        for (int j = 0; j < 30; j++) {
            int px = (start_x + j) % FACE_WIDTH;
            int py = (start_y + j/2) % FACE_HEIGHT;
            int idx = py * FACE_WIDTH + px;
            buffer[idx] = 15;
        }
    }
}
