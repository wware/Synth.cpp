<!--
pip install markdown
python -m markdown README.md > README.html
-->

Modular Music Synthesis in C++
====

Historical background
----

Way back when, one of my friends in high school was a guy named [Dave Wilson](http://www.matrixsynth.com/2010/08/rip-david-hillel-wilson-curator-of-new.html) whose father got him into 1970s-era Moog-style analog electronic music synthesizers. Dave created a museum of historical synthesizers in his home in Nashua, New Hampshire. Throughout our high school and college years, we exchanged ideas and circuits and bits of lore for various synthesizer hacks.

Music synthesizers of that era were composed of [modules](https://en.wikipedia.org/wiki/Modular_synthesizer) and were actually special-purpose [analog computers](https://en.wikipedia.org/wiki/Analog_computer), performing arithmetic operations with [integrators, summers, and other such circuits](https://courses.engr.illinois.edu/ece486/labs/lab1/analog_computer_manual.pdf). These computations can be performed digitally by a microprocessor or special-purpose digital circuit (e.g. FPGA). So Dave and I both at various points and in various contexts wrote code to do that.

Sound generation in this code is done in C++, and can run on the [Teensy 3.1 board](https://www.pjrc.com/teensy/teensy31.html) which has a 32-bit ARM microcontroller capable of running at 96 MHz.

Embodiments
----

This code has two readily available embodiments. One is the test.py script which will generate an audio file called "quux.aiff" and play the file.

The other is an easy-to-build piece of electronics using a Teensy board, some batteries, some pushbuttons, a few components, and a pair of earbuds. I call this thing a "trivisynth" because it is the most trivial piece of hardware that one could justifiably call a synthesizer.

Performance considerations
----

The ARM core on the Teensy does not have floating-point hardware, so the most frequently performed calculations should be done with [fixed-point arithmetic](https://en.wikipedia.org/wiki/Fixed-point_arithmetic). On integer machines, divide operations are also slow, so alwyas prefer a right-shift whenever possible.

An interrupt handler is used to transfer audio samples from a queue to the DAC. This should be done as quickly as possible.

If the arithmetic is too slow, the queue will underrun because the frequency of the timer interrupt is fixed. This is indicated by the LED turning on. If you see the LED, your options are to reduce the sampling rate or reduce the number of voices.

At some point I will take some time to thoroughly profile pieces of the code, particularly the Filter::step() method, and see where things should be more thoroughly optimized. The Teensy processor's instruction set includes a nice multiply-accumulate that intrigues me.

test.py
====

This script works on both OS X and Ubuntu. It generates an audio file and plays it.

trivisynth
====

I've posted this as a project on [Frizing.org](http://fritzing.org/projects/trivisynth#).

![A picture of the trivisynth](https://lh3.googleusercontent.com/tSo49FZMQ2PZuDIjJzavJvc45A1Cf91DhcYhVb45Q_BDbQXfR_f2IWK63ptriyPMP9Le3eJpGUe7yvTVp3cwlVbKMRs4iC5uUV4V62iX-wCddOKisSDtSAdTb1LvalenJMBUtVIajahGO530ErIOzeYtP671tXtWqocGeDMaA7mRfVsnrno92JqKhon-7BOq9P_FC6Z55-XAdKhgL8GJHpnfwD8sGNDwLHpy4NVnxuFKNQdN6eJF6AvQtbRDrvEYkb18sD3Jcs9x3rfgB0-k1crRgv8HBYke_NEI0gWB6ympUBT866xli2jJ0lANcaMBMdN5TXHvLdL_c87GqyJoo2x-e0y-kVUoRdw9mZqCNvfmUmKPTmw5JAi8tqjApq1GdCPvLwUhf4qQvQWkR5VH9gEd1jWx_hJVzCa-V7xPjw7ewFKIrvFGEeP0gX8Ze6JzdanAwQoAYHcecEh_QK-HDbxqsOK59r4i9nCGEB5_yx4MzjiaOvw9ubp41KQzO2ZGaDrABA=w810-h607-no)

![Circuit for the trivisynth](trivisynth/trivisynth.png)

You'll need [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html) set up in order to load the code onto your Teensy board.

Provide instructions to build the thing and a little info about how to play it.

Teensy 3.1 info
----

* [The Freescale Semiconductor web page](http://www.freescale.com/webapp/sps/site/prod_summary.jsp?code=K20_50) for the MK20DX256VLH7 processor used in the Teensy 3.1.
* [The datasheet](https://www.pjrc.com/teensy/K20P64M72SF1RM.pdf) that talks about GPIO programming starting on page 1331.
* [The Teensy 3.1 schematic](https://www.pjrc.com/teensy/schematic.html). PTB17 is the GPIO that I want to read. The Port B input register is at 0x400FF050. There is no MMU so no need for any mmap craziness.
* [Here](https://forum.pjrc.com/threads/25317-Assembly-coding-for-Teensy3-1) is a great discussion of embedding assembly in C code. Also see http://www.ethernut.de/en/documents/arm-inline-asm.html.
* [A C header file](http://www.keil.com/dd/docs/arm/freescale/kinetis/mk20d7.h) for registers in the MK20DX.
* [Some nice info](http://www.peter-cockerell.net/aalp/html/frames.html) on ARM assembly language.
* There is an [online ARM C++ compiler](http://assembly.ynh.io/).

Here is what assembly language looks like.

```c++
static int measurePeriod(void)
{
    // Step 1, set up registers
    int count = 0, x = 0, mask = 1 << 17, addr = 0x400ff050;
#   define READ_INPUT  "ldr %1, [%3]\n"     "ands %1, %1, %2\n"
#   define INC_COUNT   "add %0, %0, #1\n"
    asm volatile(
        // Step 2, if input is low go to step 7
        READ_INPUT
        "beq step7"                             "\n"
        // Step 3, wait for falling edge
        "step3:"                                "\n"

        ... lots more code ...

        "step10:"                               "\n"
        : "+r" (count), "+r" (x), "+r" (mask), "+r" (addr)
    );
    return count;
}
```
