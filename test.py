import math
import os
import sys

# Set __ARM = 0 so later it will be possible to disable assembly language.
CMD = ("g++ -Wall -g -D__ARM=0 -Iteensy -o foo test.cpp teensy/synth.cpp")
assert os.system(CMD) == 0, CMD

if 'valgrind' in sys.argv[1:]:
    CMD = "valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./foo"
else:
    CMD = "./foo"
assert os.system(CMD) == 0, CMD

if 'gnuplot' in sys.argv[1:]:
    os.system("echo \"set term png; set output 'output.png';"
        " plot 'foo.gp' using 1:3 with lines, 'foo.gp' using 1:2 with lines\" | gnuplot")
    sys.exit(0)

import aifc
fname = "quux.aiff"

if sys.platform == "darwin":
    player = "afplay"
else:
    assert sys.platform == "linux2"
    player = "play"   # apt-get install sox

def f(x):
    xhi = (x >> 8) & 0xff
    xlo = x & 0xff
    return chr(xhi) + chr(xlo)

import foo
S = foo.samples

try:
    os.unlink(fname)
except:
    pass

q = aifc.open(fname, "wb")
q.setnchannels(1)
q.setsampwidth(2)
q.setframerate(foo.sampfreq)
q.setnframes(len(S))

q.writeframes("".join(map(f, S)))
q.close()

os.system(player + " " + fname)
