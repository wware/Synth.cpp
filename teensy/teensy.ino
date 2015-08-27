#include <TimerOne.h>

#include "synth.h"
#include "voice.h"

#define KEYBOARD 1
#define NUM_KEYS 11

/** @file */

class ThreadSafeSynth : public Synth
{
    void write_sample(void) {
        cli();
        Synth::write_sample();
        sei();
    }
};

class FunctionKey : public Key
{
    void keyup(void) { /* nada */ }
    void keydown(void) {
        switch (id) {
            case 8:
                s.quiet();
                cli();
                use_synth(&s);
                sei();
                break;
            case 9:
                s2.quiet();
                cli();
                use_synth(&s2);
                sei();
                break;
            case 10:
                s3.quiet();
                cli();
                use_synth(&s3);
                sei();
                break;
        }
    }
};

Key *keyboard[NUM_KEYS];
ThreadSafeSynth s, s2, s3;
ISynth *synth_ary[3];

int8_t pitches[] = { 0, 2, 4, 5, 7, 9, 11, 12 };
extern uint32_t tune[];
uint32_t start_time;
uint32_t tune_pointer;

/**
 * The timer interrupt takes audio samples from the queue and feeds
 * them to the 12-bit DAC. If there is a queue underrun, it turns on
 * the LED briefly but visibly.
 */
void timer_interrupt(void)
{
    static uint8_t led_time;
    uint32_t x;
    if (get_synth()->get_sample(&x) == 0) {
        analogWrite(A14, x);
    } else {
        led_time = 100;
    }
    if (led_time > 0) {
        led_time--;
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

/**
 * Keyboard scanning passes thru this function because all Teensy-specific
 * code lives in this file. If you're thinking about touch-sensitive keys
 * or other exotica, this is the place for that. Also, if keys are assigned
 * special functions, that should be handled here.
 */
uint8_t read_key(uint32_t id)
{
    return (digitalReadFast(id) == LOW);
}

/**
 * Note that there are different numbers of voices assigned for the
 * different types of voice. The more complicated a voice is, the
 * less polyphony is possible. There are two ways to address this.
 *
 * * Lower the sampling rate, which adversely impacts sound quality.
 * * Speed up the code. That means doing a lot of profiling
 *   (best done with a GPIO pin and an oscilloscope in this situation)
 *   and then write tighter C++ code and possibly some assembly language.
 */
void setup() {
    uint8_t i;
    start_time = micros();
    tune_pointer = 0;
    /*
     * The more complicated a voice is, the less polyphony is possible.
     * There are two ways to address this. One is to lower the sampling
     * rate (which adversely impacts sound quality), and speeding up the
     * code. That means doing a lot of profiling (best done with a GPIO
     * pin and an oscilloscope in this situation) possibly followed by
     * tighter C++ code and possibly some assembly language.
     */
#define NUM_NOISY_VOICES  4
#define NUM_SIMPLE_VOICES  14
#define NUM_SQUARE_VOICES  8
    synth_ary[0] = &s;
    synth_ary[1] = &s2;
    synth_ary[2] = &s3;
    for (i = 0; i < NUM_NOISY_VOICES; i++)
        s.add(new NoisyVoice());
    for (i = 0; i < NUM_SIMPLE_VOICES; i++)
        s2.add(new SimpleVoice());
    for (i = 0; i < NUM_SQUARE_VOICES; i++)
        s3.add(new TwoSquaresVoice());
    s.quiet();
    use_synth_array(synth_ary, 3);
    use_synth(0);
    analogWriteResolution(12);
    Timer1.initialize((int) (1000000 * DT));
    Timer1.attachInterrupt(timer_interrupt);
    pinMode(0, INPUT_PULLUP);
    pinMode(1, INPUT_PULLUP);
    pinMode(2, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    for (i = 0; i < NUM_KEYS - 3; i++) {
        keyboard[i] = new Key();
        keyboard[i]->id = i;
        keyboard[i]->pitch = ((int8_t*) &pitches)[i];
    }
    for ( ; i < NUM_KEYS; i++) {
        keyboard[i] = new FunctionKey();
        keyboard[i]->id = i;
        keyboard[i]->pitch = 0;
    }
}

/**
 * Arduino loop function
 */
void loop(void) {
    int i;
#if KEYBOARD
    for (i = 0; i < NUM_KEYS; i++)
        keyboard[i]->check();
#else
    uint32_t msecs = (micros() - start_time) / 1000;
    if (play_tune(tune, msecs)) {
        start_time = micros();
    }
#endif
    for (i = 0; i < 64; i++)
        get_synth()->compute_sample();
}
