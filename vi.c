#include <stdio.h>
#include <stdlib.h>
#include "buf.h"

struct buf fb = {NULL, 0, 0};
struct buf ab = {NULL, 0, 0};
struct piece_table pt = {NULL};

void free_all()
{
	free(fb.b);
	free(ab.b);
	free(pt.pcs);
	free(pt.ops);
	free(pt.table);
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
	pt_init(&fb, &pt);

	/* temp testing */
	/* print piece table */
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		for (size_t j = 0; j < op->num_append; ++j) {
			struct piece *pcs = pt.pcs + *(op->append + j);
			if (pcs->buf == 'f')
				fwrite(fb.b + pcs->start, sizeof(*(fb.b)), pcs->len, stdout);
			else if (pcs->buf == 'a')
				fwrite(ab.b + pcs->start, sizeof(*(ab.b)), pcs->len, stdout);
		}
	}
	//fwrite(fb.b, sizeof(*(fb.b)), fb.len, stdout);
	printf("buffer: %zubytes\n", sizeof(*(fb.b)) * (fb.len));
	printf("lines: %zu\n", fb.lines);

	/*
	printf("line %zu:\n", fb.lines);
	char *c = vi_getline(&fb, fb.lines);
	do {
		putchar(*c);
	} while (*c != '\n' && *c++ != '\0');
	*/

	exit(EXIT_SUCCESS);
}
