#include <stdio.h>
#include <stdlib.h>
#include "buf.h"

struct buf fb = {NULL, 0, 0};

void free_all()
{
	free(fb.b);
}

void die(const char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	atexit(free_all);
	if (argc < 2)
		exit(EXIT_SUCCESS);
	vi_open(argv[1], &fb);

	fwrite(fb.b, sizeof(*(fb.b)), fb.len, stdout);
	printf("buffer: %zubytes\n", sizeof(*(fb.b)) * (fb.len));
	printf("lines: %zu\n", fb.lines);

	printf("line %zu:\n", fb.lines);
	char *c = vi_getline(&fb, fb.lines);
	do {
		putchar(*c);
	} while (*c != '\n' && *c++ != '\0');

	exit(EXIT_SUCCESS);
}
