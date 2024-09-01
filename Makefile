CC = cc
CFLAGS = -Wall -O3

all: vi

vi:
	$(CC) -o $@ $(CFLAGS) vi.c buf.c term.c
