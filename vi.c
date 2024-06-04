#include <stdio.h>
#include <stdlib.h>
#include "buf.h"

int main(int argc, char *argv[])
{
	struct fb fb = {NULL, 0};
	if (argc < 2)
		return 0;
	if (vi_open(argv[1], &fb))
		return 0;

	fwrite(fb.b, sizeof(*(fb.b)), fb.len, stdout);
	printf("buffer: %zubytes\n", sizeof(*(fb.b)) * (fb.len));

	free(fb.b);
	return 0;
}
