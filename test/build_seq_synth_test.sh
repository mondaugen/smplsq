#/bin/bash
CC=gcc
$CC synth.c seq.c test/seq_synth_test.c -g -o \
    test/seq_synth_test.bin -ljack -lpthread \
    -I/usr/local/include -I. -L/usr/local/lib $CFLAGS
