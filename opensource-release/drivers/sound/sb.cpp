#include "sound.h"
#include "io.h"

uint16_t SoundBlaster::base_port = SB_BASE_PORT;
bool SoundBlaster::available = false;


static void delay(uint32_t microseconds) {
    for (volatile uint32_t i = 0; i < microseconds * 10; i++);
}

bool SoundBlaster::reset_dsp() {
    
    outb(base_port + 0x6, 1);
    delay(10);

    
    outb(base_port + 0x6, 0);
    delay(10);

    
    for (int i = 0; i < 100; i++) {
        if (inb(base_port + 0xE) & 0x80) {  
            if (inb(base_port + 0xA) == 0xAA) {  
                return true;
            }
        }
        delay(10);
    }

    return false;
}

bool SoundBlaster::write_dsp(uint8_t value) {
    
    for (int i = 0; i < 1000; i++) {
        if ((inb(base_port + 0xC) & 0x80) == 0) {
            outb(base_port + 0xC, value);
            return true;
        }
        delay(1);
    }
    return false;
}

uint8_t SoundBlaster::read_dsp() {
    
    while ((inb(base_port + 0xE) & 0x80) == 0);
    return inb(base_port + 0xA);
}

void SoundBlaster::set_sample_rate(uint16_t rate) {
    uint8_t time_constant = 256 - (1000000 / rate);
    write_dsp(0x40);  
    write_dsp(time_constant);
}

bool SoundBlaster::initialize(uint16_t port) {
    base_port = port;

    
    if (reset_dsp()) {
        available = true;

        
        write_dsp(0xE1);  
        
        

        return true;
    }

    available = false;
    return false;
}

void SoundBlaster::play_sample(const uint8_t* data, uint32_t length, uint16_t sample_rate) {
    if (!available || length == 0) return;

    set_sample_rate(sample_rate);

    
    

    write_dsp(0x10);  

    for (uint32_t i = 0; i < length; i++) {
        write_dsp(data[i]);
        delay(1000000 / sample_rate);  
    }
}

void SoundBlaster::beep(uint16_t frequency, uint32_t duration_ms) {
    
    PCSpeaker::play_tone(frequency, duration_ms);
}

bool SoundBlaster::is_available() {
    return available;
}



void PCSpeaker::play_frequency(uint16_t frequency) {
    if (frequency == 0) {
        stop();
        return;
    }

    uint32_t divisor = 1193180 / frequency;

    
    outb(0x43, 0xB6);

    
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);
}

void PCSpeaker::stop() {
    
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

void PCSpeaker::play_tone(uint16_t frequency, uint32_t duration_ms) {
    play_frequency(frequency);

    
    for (volatile uint32_t i = 0; i < duration_ms * 1000; i++);

    stop();
}
