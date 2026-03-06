CC = gcc
CFLAGS = -g -Wall -Werror -I../src
LDFLAGS = -lSDL2 -lm

DSP_SRC = \
    ../src/common.c \
    ../src/dsp/dsp.c \
    ../src/dsp/node.c \
    ../src/dsp/nodes/dac.c \
    ../src/dsp/nodes/delay.c \
    ../src/dsp/nodes/line.c \
    ../src/dsp/nodes/math.c \
    ../src/dsp/nodes/osc.c \
    ../src/dsp/nodes/reverb.c \
    ../src/dsp/nodes/shaper.c \
    ../src/dsp/nodes/svf.c \
    ../src/lib/freeverb/freeverb.c

OBJS = $(DSP_SRC:.c=.o)

test_app: wrapper.o graph.o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

wrapper.o: wrapper.c
	$(CC) $(CFLAGS) -c $< -o $@

graph.o: graph.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f test_app wrapper.o graph.o $(OBJS)
