#ifndef VOICE_H_INCLUDED
#define VOICE_H_INCLUDED 1

#include "synth.h"


class SimpleVoice : public IVoice {
public:
    Oscillator osc1;
    ADSR adsr;

    SimpleVoice() {
        osc1.setwaveform(1);
        adsr.setA(0.1);
        adsr.setD(0.4);
        adsr.setS(0.2);
        adsr.setR(0.1);
    }
    void step(void) {
        osc1.step();
        adsr.step();
    }
    void quiet(void) {
        adsr.quiet();
    }
    bool idle(void) {
        return adsr.state() == 0;
    }
    void setfreq(float f) {
        osc1.setfreq(f);
    }
    void keydown(void) {
        adsr.keydown();
    }
    void keyup(void) {
        adsr.keyup();
    }
    int32_t output(void) {
        return ((int32_t)adsr.output() * osc1.output()) >> 12;
    }
    void ioctl(uint32_t param, uint32_t value) {
        // do nothing for now
    }
};

class TwoSquaresVoice : public IVoice {
public:
    Oscillator osc1, osc2;
    ADSR adsr, adsr2;

    TwoSquaresVoice() {
        osc1.setwaveform(2);  // I <3 square waves
        osc2.setwaveform(2);
        adsr.setA(0.3);
        adsr.setD(0.4);
        adsr.setS(0.2);
        adsr.setR(0.1);
        adsr2.setA(0.01);
        adsr2.setD(0.1);
        adsr2.setS(0.1);
        adsr2.setR(0.1);
    }
    void step(void) {
        osc1.step();
        osc2.step();
        adsr.step();
        adsr2.step();
    }
    void quiet(void) {
        adsr.quiet();
        adsr2.quiet();
    }
    bool idle(void) {
        return adsr.state() == 0 && adsr2.state() == 0;
    }
    void setfreq(float f) {
        osc1.setfreq(f);
        osc2.setfreq(f / 2);
    }
    void keydown(void) {
        adsr.keydown();
        adsr2.keydown();
    }
    void keyup(void) {
        adsr.keyup();
        adsr2.keyup();
    }
    int32_t output(void) {
        return (((int32_t)adsr.output() * osc1.output()) >> 14) +
            (((int32_t)adsr2.output() * osc2.output()) >> 14);
    }
    void ioctl(uint32_t param, uint32_t value) {
        // do nothing for now
    }
};

class NoisyVoice : public IVoice {
public:
    Oscillator osc1, osc2, osc3;
    ADSR adsr, adsr2;
    Filter filt;
    uint32_t _f;

    NoisyVoice() {
        filt.setQ(6);
        osc1.setwaveform(1);
        osc2.setwaveform(2);
        osc3.setwaveform(2);
        adsr.setA(0.03);
        adsr.setD(0.7);
        adsr.setS(0.4);
        adsr.setR(0.1);
        adsr2.setA(0.03);
        adsr2.setD(0.6);
        adsr2.setS(0.0);
        adsr2.setR(0.6);
    }
    void step(void) {
        uint32_t __f = _f;
        osc1.step();
        osc2.step();
        osc3.step();
        adsr.step();
        adsr2.step();
        filt.setF((__f * adsr2.output()) >> 12);
        filt.step((osc1.output() + osc2.output() + osc3.output()) >> 1);
    }
    bool idle(void) {
        return adsr.state() == 0;
    }
    void setfreq(float f) {
        _f = f;
        osc1.setfreq(f);
        osc2.setfreq(f + small_random());
        osc3.setfreq(f / 2 + 0.5 * small_random());
    }
    void quiet(void) {
        adsr.quiet();
        adsr2.quiet();
    }
    void keydown(void) {
        adsr.keydown();
        adsr2.keydown();
    }
    void keyup(void) {
        adsr.keyup();
        adsr2.keyup();
    }
    int32_t output(void) {
        int32_t x;
        x = filt.bandpass();
        x += filt.highpass() >> 1;
        return ((int32_t)adsr.output() * x) >> 13;
    }
    void ioctl(uint32_t param, uint32_t value) {
        // do nothing for now
    }
};

#endif     // VOICE_H_INCLUDED
