#include <stdio.h>
#include <stdlib.h>
#include "buf.h"

struct buf fb = {NULL, 0};
struct buf ab = {NULL, 0};
struct piece_table pt = {NULL};

void free_all()
{
	free(fb.b);
	free(ab.b);
	for (size_t i = 0; i < pt.num_ops; ++i) {
		free((pt.ops + i)->pcs);
		free((pt.ops + i)->del);
	}
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
	pt_insert("test 1\n", 0, &fb, &ab, &pt);
	pt_insert("test 2\n", 12, &fb, &ab, &pt);
	pt_insert("test 3\n", 19, &fb, &ab, &pt);
	pt_insert(" 4 &", 4, &fb, &ab, &pt);
	pt_insert(" 5 &", 8, &fb, &ab, &pt);

	printf("total lines: %lu\n", pt.lines);
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt.pcs + *(op->pcs + j);
			if (pc->buf == 'f')
				fwrite(fb.b + pc->start, sizeof(*(fb.b)), pc->len, stdout);
			else if (pc->buf == 'a')
				fwrite(ab.b + pc->start, sizeof(*(ab.b)), pc->len, stdout);
		}
	}
	puts("\n####################\n\n");
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		printf("operation lines: %lu\n", op->lines);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt.pcs + *(op->pcs + j);
			if (pc->buf == 'f')
				fwrite(fb.b + pc->start, sizeof(*(fb.b)), pc->len, stdout);
			else if (pc->buf == 'a')
				fwrite(ab.b + pc->start, sizeof(*(ab.b)), pc->len, stdout);
		}
		puts("\n####################");
	}
	puts("\n\n");
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt.pcs + *(op->pcs + j);
			printf("piece lines: %lu\n", pc->lines);
			if (pc->buf == 'f')
				fwrite(fb.b + pc->start, sizeof(*(fb.b)), pc->len, stdout);
			else if (pc->buf == 'a')
				fwrite(ab.b + pc->start, sizeof(*(ab.b)), pc->len, stdout);
			puts("\n####################");
		}
	}
	puts("\n\n__output__");

	/* print piece table */
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt.pcs + *(op->pcs + j);
			if (pc->buf == 'f')
				fwrite(fb.b + pc->start, sizeof(*(fb.b)), pc->len, stdout);
			else if (pc->buf == 'a')
				fwrite(ab.b + pc->start, sizeof(*(ab.b)), pc->len, stdout);
		}
	}
	/* temp testing ends */

	exit(EXIT_SUCCESS);
}
