#ifndef BUF_H
#define BUF_H

#define BUF_SIZE 16384

struct buf {
	char *b;
	size_t len;
	size_t lines;
};

/*
 * buf: 'f' = file buffer
 *      'a' = append buffer
 */
struct piece {
	char buf;
	size_t start;
	size_t len;
};

struct operation {
	size_t *del;
	size_t *append;
	size_t num_del;
	size_t num_append;
};

struct piece_table {
	struct piece *pcs;
	struct operation *ops;
	size_t *table;
	size_t undo;
	size_t num_pcs;
	size_t num_ops;
	size_t num_table;
};

void pt_init(struct buf *fb, struct piece_table *pt);
void vi_open(const char *f, struct buf *fb);
char *vi_getline(const struct buf *fb, size_t n);

#endif
