#ifndef SOUND_H
#define SOUND_H

#include "types.h"


#define SB_BASE_PORT 0x220

class SoundBlaster {
public:
    static bool initialize(uint16_t base_port = SB_BASE_PORT);
    static void play_sample(const uint8_t* data, uint32_t length, uint16_t sample_rate);
    static void beep(uint16_t frequency, uint32_t duration_ms);
    static bool is_available();

private:
    static bool reset_dsp();
    static bool write_dsp(uint8_t value);
    static uint8_t read_dsp();
    static void set_sample_rate(uint16_t rate);

    static uint16_t base_port;
    static bool available;
};


class PCSpeaker {
public:
    static void play_tone(uint16_t frequency, uint32_t duration_ms);
    static void stop();

private:
    static void play_frequency(uint16_t frequency);
};

#endif
