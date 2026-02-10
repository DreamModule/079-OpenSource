#include "mouse.h"
#include "graphics.h"
#include "io.h"


#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64



#ifndef likely
#define likely(x)   __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

MouseState Mouse::state = {160, 100, 0, false, false, false};
MouseState Mouse::prev_state = {160, 100, 0, false, false, false};
uint8_t Mouse::packet[4] = {0, 0, 0, 0};
uint8_t Mouse::packet_index = 0;
uint8_t Mouse::packet_size = 3;  
bool Mouse::initialized = false;
bool Mouse::scroll_wheel_enabled = false;
int8_t Mouse::scroll_accumulator = 0;
bool Mouse::click_consumed[3] = {false, false, false};
uint8_t Mouse::button_stability[3] = {0, 0, 0};
uint8_t Mouse::resync_attempts = 0;
uint16_t Mouse::packets_processed = 0;


static void wait_input() {
    for (int i = 0; i < 100000; i++) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) {
            return;
        }
    }
}

static void wait_output() {
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

void Mouse::send_command(uint8_t cmd) {
    wait_input();
    outb(PS2_COMMAND_PORT, 0xD4);
    wait_input();
    outb(PS2_DATA_PORT, cmd);
}

uint8_t Mouse::read_data() {
    wait_output();
    return inb(PS2_DATA_PORT);
}

void Mouse::initialize() {
    
    
    for (int i = 0; i < 100; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if (status & 0x01) {
            inb(PS2_DATA_PORT);  
        } else {
            break;
        }
    }

    
    wait_input();
    outb(PS2_COMMAND_PORT, 0xAD);  
    wait_input();
    outb(PS2_COMMAND_PORT, 0xA7);  

    
    for (int i = 0; i < 10; i++) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            inb(PS2_DATA_PORT);
        } else {
            break;
        }
    }

    
    wait_input();
    outb(PS2_COMMAND_PORT, 0xA8);

    
    wait_input();
    outb(PS2_COMMAND_PORT, 0x20);
    uint8_t config = read_data();

    
    
    
    
    
    config |= 0x03;   
    config &= ~0x30;  

    wait_input();
    outb(PS2_COMMAND_PORT, 0x60);
    wait_input();
    outb(PS2_DATA_PORT, config);

    
    send_command(0xFF);  
    read_data();  
    
    for (int i = 0; i < 10; i++) {
        wait_output();
        uint8_t result = inb(PS2_DATA_PORT);
        if (result == 0xAA) break;  
        if (result == 0xFC) break;  
    }
    
    wait_output();
    inb(PS2_DATA_PORT);

    
    send_command(0xF6);  
    read_data();  

    
    
    scroll_wheel_enabled = enable_scroll_wheel();
    if (scroll_wheel_enabled) {
        packet_size = 4;  
    } else {
        packet_size = 3;  
    }

    
    send_command(0xF3);  
    read_data();  
    send_command(100);   
    read_data();  

    
    send_command(0xF4);
    read_data();  

    
    wait_input();
    outb(PS2_COMMAND_PORT, 0xAE);  

    
    for (int i = 0; i < 20; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if (!(status & 0x01)) break;  
        if (status & 0x20) {
            
            inb(PS2_DATA_PORT);
        } else {
            
            break;
        }
    }

    
    initialized = true;
    packet_index = 0;
    packets_processed = 0;
    resync_attempts = 0;

    for (int i = 0; i < 3; i++) {
        button_stability[i] = 0;
        click_consumed[i] = false;
        packet[i] = 0;
    }

    
    state.x = 160;
    state.y = 100;
    state.left_button = false;
    state.right_button = false;
    state.middle_button = false;

    prev_state = state;
}



bool Mouse::enable_scroll_wheel() {
    
    
    

    
    send_command(0xF3);
    read_data();  
    send_command(200);
    read_data();  

    
    send_command(0xF3);
    read_data();  
    send_command(100);
    read_data();  

    
    send_command(0xF3);
    read_data();  
    send_command(80);
    read_data();  

    
    send_command(0xF2);
    read_data();  
    uint8_t device_id = read_data();

    
    
    return (device_id == 0x03 || device_id == 0x04);
}


int8_t Mouse::get_scroll_delta() {
    int8_t delta = scroll_accumulator;
    scroll_accumulator = 0;
    return delta;
}

void Mouse::handle_packet(uint8_t byte) {
    
    
    
    

    
    if (packet_index == 0) {
        if (!(byte & 0x08)) {
            
            
            resync_attempts++;
            return;
        }
        
        if ((byte & 0xC0) == 0xC0) {
            
            resync_attempts++;
            return;
        }
    }

    packet[packet_index] = byte;
    packet_index++;

    if (packet_index == packet_size) {
        
        uint8_t flags = packet[0];
        int16_t dx = packet[1];
        int16_t dy = packet[2];

        
        if (packet_size == 4) {
            
            int8_t scroll_delta = (int8_t)packet[3];
            scroll_accumulator += scroll_delta;
            state.scroll = scroll_delta;
        } else {
            state.scroll = 0;
        }

        
        prev_state = state;

        
        bool raw_left = (flags & 0x01) != 0;
        bool raw_right = (flags & 0x02) != 0;
        bool raw_middle = (flags & 0x04) != 0;

        
        
        if (raw_left && !state.left_button) {
            
            if (button_stability[0] > 0) {
                state.left_button = true;
                click_consumed[0] = false;
                button_stability[0] = 0;
            } else {
                button_stability[0]++;
            }
        } else if (!raw_left && state.left_button) {
            
            state.left_button = false;
            click_consumed[0] = false;
            button_stability[0] = 0;
        } else {
            button_stability[0] = 0;
        }

        
        if (raw_right && !state.right_button) {
            if (button_stability[1] > 0) {
                state.right_button = true;
                click_consumed[1] = false;
                button_stability[1] = 0;
            } else {
                button_stability[1]++;
            }
        } else if (!raw_right && state.right_button) {
            state.right_button = false;
            click_consumed[1] = false;
            button_stability[1] = 0;
        } else {
            button_stability[1] = 0;
        }

        
        if (raw_middle && !state.middle_button) {
            if (button_stability[2] > 0) {
                state.middle_button = true;
                click_consumed[2] = false;
                button_stability[2] = 0;
            } else {
                button_stability[2]++;
            }
        } else if (!raw_middle && state.middle_button) {
            state.middle_button = false;
            click_consumed[2] = false;
            button_stability[2] = 0;
        } else {
            button_stability[2] = 0;
        }

        
        if (flags & 0x10) dx |= 0xFF00;  
        if (flags & 0x20) dy |= 0xFF00;  

        
        state.x += dx;
        state.y -= dy; 

        
        if (state.x < 0) state.x = 0;
        if (state.x >= 320) state.x = 319;
        if (state.y < 0) state.y = 0;
        if (state.y >= 200) state.y = 199;

        packet_index = 0;

        
        packets_processed++;
        
        if (packets_processed == 0xFFFF) {
            packets_processed = 1000;  
        }
    }
}

void Mouse::update() {
    if (!initialized) return;

    
    
    for (int i = 0; i < 30; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);

        
        if (!(status & 0x01)) {
            
            break;
        }

        
        if (status & 0x20) {
            
            uint8_t data = inb(PS2_DATA_PORT);
            handle_packet(data);
        } else {
            
            break;
        }
    }

    
    
    
    static uint16_t last_packets = 0;
    static uint8_t stall_counter = 0;

    if (packets_processed == last_packets && packet_index > 0) {
        stall_counter++;
        if (stall_counter > 100) {
            
            packet_index = 0;
            stall_counter = 0;
            resync_attempts++;
        }
    } else {
        stall_counter = 0;
        last_packets = packets_processed;
    }
}

MouseState Mouse::get_state() {
    return state;
}

bool Mouse::is_button_pressed(uint8_t button) {
    switch (button) {
        case 0: return state.left_button;
        case 1: return state.right_button;
        case 2: return state.middle_button;
        default: return false;
    }
}

bool Mouse::was_button_clicked(uint8_t button) {
    
    
    if (button > 2) return false;

    bool clicked = false;
    switch (button) {
        case 0: clicked = prev_state.left_button && !state.left_button; break;
        case 1: clicked = prev_state.right_button && !state.right_button; break;
        case 2: clicked = prev_state.middle_button && !state.middle_button; break;
    }

    
    if (clicked && !click_consumed[button]) {
        click_consumed[button] = true;
        return true;
    }

    return false;
}

void Mouse::draw_cursor(uint8_t color) {
    if (!initialized) return;

    
    for (int dy = 0; dy < 5; dy++) {
        for (int dx = 0; dx < 3; dx++) {
            if (dx <= dy) {
                Graphics::put_pixel(state.x + dx, state.y + dy, color);
            }
        }
    }
}

// FIXME (upcoming in new update)
void Mouse::hide_cursor() {
    
}
