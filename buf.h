#ifndef BUF_H
#define BUF_H

#define BUF_SIZE 16384

struct buf {
	char *b;
	size_t len;
};

struct piece {
	size_t start;
	size_t len;
	size_t lines;
};

struct operation {
	size_t *del;
	size_t *pcs;
	size_t num_del;
	size_t num_pcs;
	size_t len;
	size_t lines;
};

struct piece_table {
	struct piece *pcs;
	struct operation *ops;
	size_t *table;
	size_t num_pcs;
	size_t num_ops;
	size_t num_table;
	size_t len;
	size_t lines;
};

void pt_insert(char *b, size_t pos, struct buf *eb, struct piece_table *pt);
void pt_init(struct buf *eb, struct piece_table *pt);
void vi_open(const char *f, struct buf *eb);
void vi_save(const char *f, struct piece_table *pt, struct buf *eb);

#endif
