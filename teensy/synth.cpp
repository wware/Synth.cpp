#if ! __ARM
#include <stdio.h>
#endif
#include <math.h>

#include "synth.h"

#define GOAL     (0.95 * (1 << 28))

/* 1 / (1 - 1/e), because exponential */
#define BIGGER (1.5819767 * (1 << 28))

#define RARE  500
#define NOT_TOO_SMALL(x)    MAX(x, 0.01)

#define KEYDOWN_HYSTERESIS 10

ISynth *_synth = NULL;
uint8_t (*_read_key)(uint32_t) = NULL;

void use_read_key(uint8_t (*rk)(uint32_t))
{
    _read_key = rk;
}

void use_synth(ISynth *s)
{
    _synth = s;
}

ISynth * get_synth(void)
{
    return _synth;
}

float small_random() {
    return -2.0 + 0.01 * (rand() % 400);
}

void assertion(int cond, const char *strcond,
               const char *file, const int line)
{
#if ! __ARM
    if (!cond) {
        fprintf(stderr,
                "%s(%d) ASSERTION FAILED: %s\n",
                file, line, strcond);
        exit(1);
    }
#endif
}


uint8_t Queue::read(uint32_t *x) {
    if (empty()) return 1;
    *x = buffer[rpointer];
    rpointer = (rpointer + 1) & (BUFSIZE - 1);
    return 0;
}

uint8_t Queue::write(uint32_t x) {
    if (full()) return 1;
    buffer[wpointer] = x;
    wpointer = (wpointer + 1) & (BUFSIZE - 1);
    return 0;
}

void Synth::quiet(void) {
    uint8_t i;
    for (i = 0; i < 100; i++)
        assignments[i] = NULL;
    for (i = 0; i < num_voices; i++)
        voices[i]->quiet();
    next_voice_to_assign = 0;
}

void Synth::keydown(int8_t pitch) {
    IVoice *v = assignments[pitch + 50];
    if (v != NULL) {
        // we already have a voice for this pitch
        if (!v->idle())
            // it's already active, no keydown needed
            return;
    } else {
        v = get_next_available_voice(pitch);
        v->setfreq(261.6255653f * pow(1.059463094359f, pitch));
    }
    v->keydown();
}

void Synth::keyup(int8_t pitch) {
    IVoice *v = assignments[pitch + 50];
    if (v != NULL) {
        v->keyup();
        assignments[pitch + 50] = NULL;
    }
}

void Synth::compute_sample(void) {
    if (!again) {
        uint8_t i;
        for(i = 0; i < num_voices; ++i){
            voices[i]->step();
        }
        x = get_12_bit_value();
    }
    write_sample();
}

uint32_t Synth::get_12_bit_value(void)
{
    uint8_t i, n = 255 / num_voices;
    int32_t x = 0;
    for(i = 0; i < num_voices; ++i){
        x += voices[i]->output();
    }
    x = (x * n) >> 8;
    ASSERT(x < 0x800);
    ASSERT(x >= -0x800);
    return (x + 0x800) & 0xFFF;
}

IVoice * Synth::get_next_available_voice(int8_t pitch) {
    uint32_t i, j, found;
    int32_t n;
    IVoice *v;
    // Look for the least recently used voice whose state is zero.
    i = next_voice_to_assign;
    for (j = 0, found = 0; j < num_voices; j++) {
        if (voices[i]->idle()) {
            found = 1;
            break;
        }
        // wrap aropund
        if (++i == num_voices) i = 0;
    }
    // If none is found, just grab the least recently used voice.
    if (found)
        v = voices[i];
    else
        v = voices[next_voice_to_assign];

    next_voice_to_assign++;
    if (next_voice_to_assign == num_voices) {
        // wrap aropund
        next_voice_to_assign = 0;
    }

    // does some other key already have this voice? If so, remove
    // the voice from that other key
    for (n = -50; n < 50; n++) {
        IVoice *old_voice = assignments[n + 50];
        if (pitch != n && old_voice == v) {
            assignments[n + 50] = NULL;
            break;
        }
    }
    assignments[pitch + 50] = v;
    return v;
}


void ADSR::rare_step(void) {
    // Done rarely, so float arithmetic OK here
    float next_value, h;
    switch (_state) {
    case 1:
        h = exp(-RARE * DT / attack);
        next_value = h * _value + (1.0 - h) * BIGGER;
        if (next_value > GOAL) {
            _state = 2;
            next_value = GOAL;
        }
        break;
    case 2:
        h = exp(-RARE * DT / decay);
        next_value = h * _value + (1.0 - h) * sustain;
        break;
    default:
    case 0:
        h = exp(-RARE * DT / release);
        next_value = h * _value;
        if (next_value < 1)
            next_value = 0;
        break;
    }
    dvalue = (next_value - _value) / RARE;
}

void ADSR::setA(float a) {
    attack = NOT_TOO_SMALL(a);
}

void ADSR::setD(float d) {
    decay = NOT_TOO_SMALL(d);
}

void ADSR::setS(float s) {
    sustain = GOAL * s;
}

void ADSR::setR(float r) {
    release = NOT_TOO_SMALL(r);
}

void ADSR::keydown(void) {
    _state = 1;
    count = 0;
}

void ADSR::keyup(void) {
    _state = 0;
}

void ADSR::step(void) {
    if (count == 0) {
        rare_step();
    }
    count = (count + 1) % RARE;
    _value += dvalue;
}

int32_t Oscillator::output(void) {
    switch (waveform) {
    default:
    case 0:
        // ramp
        if (phase < 0x80000000) return (((int32_t) phase) >> 20);
        else return (((int32_t) phase) >> 20) - 4096;
        break;
    case 1:
        // triangle
        switch (phase >> 30) {
        case 0:
            return (phase >> 19);
        case 1:
            return -(phase >> 19) + 4096;
        case 2:
            return -(phase >> 19) + 4096;
        default:
            return (phase >> 19) - 8192;
        }
        break;
    case 2:
        // square
        if (phase == 0) return 0;
        if (phase < 0x80000000) return 0x7ff;
        return -0x800;
        break;
    }
}

void Filter::compute_two_k(void) {
    // k needs to scale with frequeuncy
    two_k = (2 * _k * _f) >> 12;
}

void Filter::setF(uint32_t f) {
    const int32_t m = 6 * 3.1415926 * (1 << 20) * DT;
    _f = f;
    w0dt = m * f;
    compute_two_k();
}

void Filter::setQ(float q) {
    const float kmin = 0.1;
    float _fk;
    _fk = 1.0 / q;
    if (_fk < kmin) _fk = kmin;   // stability
    _k = _fk * (1 << 16);
    compute_two_k();
}

void Filter::step(int32_t x) {
    int32_t y = x >> 2;
    y -= ((int32_t)two_k * integrator1) >> 12;
    y -= integrator2;
    integrator2 = clip(integrator2 + (((int32_t)w0dt * integrator1) >> 20));
    integrator1 = clip(integrator1 + (((int32_t)w0dt * u) >> 20));
    u = clip(y);
}

void Key::check(void) {
    if (_read_key != NULL && _read_key(id)) {
        if (state) {
            count = 0;
        } else {
            if (count < KEYDOWN_HYSTERESIS) {
                count++;
                if (count == KEYDOWN_HYSTERESIS) {
                    state = 1;
                    count = 0;
                    if (_synth != NULL) _synth->keydown(pitch);
                }
            }
        }
    } else {
        if (!state) {
            count = 0;
        } else {
            if (count < KEYDOWN_HYSTERESIS) {
                count++;
                if (count == KEYDOWN_HYSTERESIS) {
                    state = 0;
                    count = 0;
                    if (_synth != NULL) _synth->keyup(pitch);
                }
            }
        }
    }
}
