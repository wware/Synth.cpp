#ifndef SYNTH_H_INCLUDED
#define SYNTH_H_INCLUDED 1

#include <stdlib.h>

#ifndef __ARM
#define __ARM 1
#endif


#include <stdint.h>

#define SAMPLING_RATE   40000
#define DT (1.0 / SAMPLING_RATE)

#define ASSERT(cond)    assertion(cond, #cond, __FILE__, __LINE__)
extern void assertion(int cond, const char *strcond,
                      const char *file, const int line);

extern float small_random();
extern int32_t mult_signed(int32_t x, int32_t y);
extern int32_t mult_unsigned(uint32_t x, uint32_t y);
extern int32_t mult_unsigned_signed(uint32_t x, int32_t y);

/** Buffer for the Queue class. This MUST be a power of 2. */
#define BUFSIZE 1024

// Used a lot for fixed-point arithmetic.
#define UNIT     0x100000000LL

#define MIN(x, y)   (((x) < (y)) ? (x) : (y))
#define MAX(x, y)   (((x) > (y)) ? (x) : (y))

// #define MULSHIFT32(x, y)  ((((int64_t) x) * y) >> 32)
#define ADDCLIP(x, y)   clip(((int64_t) x) + ((int64_t) y))

inline int32_t clip(int64_t x) {
    return MAX(-0x80000000LL, MIN(0x7fffffffLL, x));
}

class IVoice {
public:
    virtual ~IVoice() {}   // http://stackoverflow.com/questions/318064
    virtual void quiet(void) = 0;
    virtual void step(void) = 0;
    virtual void setfreq(float f) = 0;
    virtual void keydown(void) = 0;
    virtual void keyup(void) = 0;
    virtual int32_t output(void) = 0;
    virtual bool idle(void) = 0;
};

class ISynth {
public:
    virtual ~ISynth() {}
    virtual void quiet(void) = 0;
    virtual void add(IVoice *voice) = 0;
    virtual void keydown(int8_t pitch) = 0;
    virtual void keyup(int8_t pitch) = 0;
    virtual uint8_t get_sample(uint32_t *x) = 0;
    virtual void write_sample(void) = 0;
    virtual void compute_sample(void) = 0;
};

extern void use_read_key(uint8_t (*rk)(uint32_t));
extern void use_synth(ISynth *s);
extern ISynth * get_synth(void);

/**
 * A queue containing unsigned 32-bit samples. WARNING: This class
 * provides NO protection against interrupts. That must be done by
 * the user:
 *
 * ~~~
 * void example_usage(void) {
 *     uint8_t r;
 *     uint32_t x;
 *     x = next_audio_sample;
 *     cli();
 *     r = queue.write(x);
 *     sei();
 *     // handle case where r != 0
 * }
 * ~~~
 *
 * Internal implementation is a fixed-size circular buffer.
 */
class Queue
{
    int wpointer, rpointer;
    uint32_t buffer[BUFSIZE];
    inline int size(void) {
        return (wpointer + BUFSIZE - rpointer) & (BUFSIZE - 1);
    }
    inline int empty(void) {
        return size() == 0;
    }
    inline int full(void) {
        return size() == BUFSIZE - 1;
    }
public:
    Queue() {
        wpointer = rpointer = 0;
    }
    /**
     * Read a sample from the queue.
     * @param x a pointer to the variable to store the sample in
     * @return 0 if read is successful, 1 if queue is empty.
     */
    uint8_t read(uint32_t *x);
    /**
     * Write a sample to the queue.
     * @param x the sample to be written
     * @return 0 if write is successful, 1 if queue is full.
     */
    uint8_t write(uint32_t x);
};

class Synth : public ISynth {
    IVoice * voices[32];
    IVoice * assignments[100];
    uint32_t num_voices, next_voice_to_assign;
    IVoice * get_next_available_voice(int8_t pitch);
    Queue samples;
    uint32_t x, again;

    uint32_t get_12_bit_value(void);

public:
    Synth() {
        int i;
        again = num_voices = 0;
        next_voice_to_assign = 0;
        for (i = 0; i < 100; i++) {
            assignments[i] = NULL;
        }
    }

    void add(IVoice *voice) {
        voices[num_voices++] = voice;
    }

    void quiet(void);

    // Pitch is in half-tones, middle C is at 0.
    void keydown(int8_t pitch);
    void keyup(int8_t pitch);

    uint8_t get_sample(uint32_t *x) {
        return samples.read(x);
    }

    // On a Teensy, you want to overload this method with a version
    // where interrupts are disabled
    void write_sample(void) {
        again = samples.write(x);
    }

    void compute_sample(void);
};

class ADSR {
    uint64_t _value;
    int64_t dvalue;
    uint32_t count;
    uint8_t _state;
    float attack, decay, sustain, release;

    void rare_step(void);

public:
    ADSR() {
        _state = _value = dvalue = count = 0;
    }

    void setA(float a);
    void setD(float d);
    void setS(float s);
    void setR(float r);

    uint32_t state() {
        return _state;
    }
    void quiet(void) {
        _state = 0;
        _value = 0;
        dvalue = 0;
    }
    uint32_t output() {
        return _value >> 32;
    }
    void keydown(void);
    void keyup(void);

    void step(void);
};

class Filter {
    int32_t integrator1, integrator2, u;
    uint32_t w0dt, two_k, _f, _k;

    void compute_two_k(void);

public:
    Filter() {
        integrator1 = integrator2 = u = 0;
    }

    void setF(uint32_t f);
    void setQ(float q);
    void step(int32_t x);

    int32_t highpass(void) {
        return u;
    }
    int32_t bandpass(void) {
        return integrator1;
    }
    int32_t lowpass(void) {
        return integrator2;
    }
};

class Oscillator {
    uint32_t phase, dphase, waveform;

public:
    Oscillator() {
        waveform = 1;
    }

    void setfreq(float f) {
        dphase = (int32_t)(UNIT * f / SAMPLING_RATE);
    }
    void setwaveform(int32_t x) {
        // 0 ramp, 1 triangle, 2 square
        waveform = x;
    }

    void step(void) {
        phase += dphase;
    }
    uint32_t get_phase(void) {
        return phase;
    }
    int32_t output(void);
};

/**
 * Representation of a key on the keyboard. Handles keyboard scanning
 * and issuing keydown/keyup events to an ISynth instance.
 */
class Key {
public:
    /**
     * A numerical index of this key, used to identify it for keyboard scanning.
     */
    uint32_t id;
    /**
     * 1 if the key is pressed/touched, 0 otherwise.
     */
    uint32_t state;
    /**
     * This counter is used for hysteresis (debouncing).
     */
    uint32_t count;
    /**
     * An integer, increments for each half-tone in pitch.
     */
    int8_t pitch;
    /**
     * The voice being used to sound this key.
     */
    IVoice *voice;

    Key() {
        count = state = 0;
        voice = NULL;
    }
    /**
     * Checks to see if this key is being pressed/touched.
     */
    void check(void);
};

#endif     // SYNTH_H_INCLUDED
