#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"



struct MouseState {
    int32_t x;
    int32_t y;
    int8_t scroll;  
    bool left_button;
    bool right_button;
    bool middle_button;
} __attribute__((packed));  

class Mouse {
public:
    static void initialize();
    static void update();
    static MouseState get_state();
    static bool is_button_pressed(uint8_t button); 
    static bool was_button_clicked(uint8_t button);

    
    static int8_t get_scroll_delta();  
    static bool has_scroll_wheel() { return scroll_wheel_enabled; }

    
    static uint16_t get_packets_processed() { return packets_processed; }
    static uint8_t get_resync_attempts() { return resync_attempts; }
    static bool is_initialized() { return initialized; }

    
    static void draw_cursor(uint8_t color);
    static void hide_cursor();

private:
    static void send_command(uint8_t cmd);
    static uint8_t read_data();
    static void handle_packet(uint8_t byte);
    static bool enable_scroll_wheel();  

    static MouseState state;
    static MouseState prev_state;
    static uint8_t packet[4];  
    static uint8_t packet_index;
    static uint8_t packet_size;  
    static bool initialized;
    static bool scroll_wheel_enabled;
    static int8_t scroll_accumulator;  
    static bool click_consumed[3];  
    static uint8_t button_stability[3];  
    static uint8_t resync_attempts;  
    static uint16_t packets_processed;  
};

#endif
