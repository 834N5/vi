#ifndef BUF_H
#define BUF_H

#include <stdbool.h>
#define BUF_SIZE 16384

struct buf {
	char *b;
	size_t len;
	size_t lines;
};

/*
 * b: 0 = file buffer
 *    1 = append buffer
 */
struct piece {
	bool b;
	size_t start;
	size_t len;
};

struct operation {
	size_t *del;
	size_t *append;
};

struct piece_table {
	struct piece *pcs;
	struct operation *ops;
	size_t *table;
	size_t *undo_stack;
};

void vi_open(const char *f, struct buf *fb);

char *vi_getline(const struct buf *fb, size_t n);

#endif
