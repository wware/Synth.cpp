#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "teensy/synth.h"
#include "teensy/voice.h"

#define NUM_VOICES 8

FILE *outf, *gp_outf;
int t;
Synth s;
ISynth *synth_ary[NUM_VOICES];
extern uint32_t tune[];

int main(void)
{
    synth_ary[0] = &s;
    use_synth_array(synth_ary, NUM_VOICES);
    use_synth(0);

    gp_outf = fopen("foo.gp", "w");

    outf = fopen("foo.py", "w");
    fprintf(outf, "sampfreq = %lf\n", (double) SAMPLING_RATE);
    fprintf(outf, "samples = [\n");

    for (int i = 0; i < NUM_VOICES; i++) {
        s.add(new NoisyVoice());
        // s.add(new TwoSquaresVoice());
        // s.add(new SimpleVoice());
    }

    for (t = 0; 1; t++) {
        int32_t y;
        uint32_t msecs = (1000 * t) / SAMPLING_RATE;
        if (play_tune(tune, msecs)) break;
        s.compute_sample();
        ASSERT(s.get_sample((uint32_t *) &y) == 0);
        fprintf(outf, "%u,\n", (uint16_t) (y << 6));

        /* Numbers for Gnuplot */
        fprintf(gp_outf, "%d %d\n", t, (int) y);
    }

    fprintf(outf, "]\n");
    fclose(outf);
    fclose(gp_outf);
}
