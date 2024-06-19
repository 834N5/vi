#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "vi.h"

#define LAST_OP (*(pt->ops + pt->num_ops - 1))
#define LAST_OP_PC (*(LAST_OP.pcs + LAST_OP.num_pcs - 1))
#define LAST_PC (*(pt->pcs + pt->num_pcs - 1))
#define LAST_TABLE (*(pt->table + pt->num_table - 1))

/* add piece to latest operation */
void op_add_piece(char buf, size_t start, size_t len, struct piece_table *pt)
{
	void *ptr;

	ptr = realloc(pt->pcs, sizeof(*pt->pcs) * ++pt->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	pt->pcs = ptr;
	LAST_PC.buf = buf;
	LAST_PC.start = start;
	LAST_PC.len = len;

	ptr = realloc(LAST_OP.pcs, sizeof(*(LAST_OP.pcs)) * ++LAST_OP.num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	LAST_OP.pcs = ptr;
	LAST_OP_PC = pt->num_pcs - 1;
}

/* insert text in a specified position in the piece table */
void pt_insert(char *b, size_t pos, struct buf *ab, struct piece_table *pt)
{
	size_t len = strlen(b);
	void *ptr;

	ptr = realloc(ab->b, sizeof(ab->b) * (ab->len + len));
	if (ptr == NULL)
		die("realloc failed");
	ab->b = ptr;
	memcpy(ab->b + ab->len, b, len);
	ab->len += len;
	/* ab.lines has not been touched yet */

	/* implement specified position later */
	//pos = 0;

	ptr = realloc(pt->ops, sizeof(*pt->ops) * ++pt->num_ops);
	if (ptr == NULL)
		die("realloc failed");
	pt->ops = ptr;
	LAST_OP.pcs = NULL;
	LAST_OP.num_pcs = 0;
	LAST_OP.del = NULL;
	LAST_OP.num_del = 0;
	op_add_piece('a', ab->len - len, len, pt);

	ptr = realloc(pt->table, sizeof(*pt->table) * ++pt->num_table);
	if (ptr == NULL)
		die("realloc failed");
	pt->table = ptr;
	LAST_TABLE = pt->num_ops - 1;

	/* implement undo later */
	//pt->undo = 0;
}

/* initialise piece table */
void pt_init(struct buf *fb, struct piece_table *pt)
{
	void *ptr;
	if (fb->b == NULL)
		return;

	if ((ptr = malloc(sizeof(*pt->table))) == NULL)
		die("malloc failed");
	pt->table = ptr;
	*pt->table = 0;
	++pt->num_table;

	if ((ptr = malloc(sizeof(*pt->ops))) == NULL)
		die("malloc failed");
	pt->ops = ptr;
	++pt->num_ops;
	pt->ops->pcs = NULL;
	pt->ops->num_pcs = 0;
	pt->ops->del = NULL;
	pt->ops->num_del = 0;
	op_add_piece('f', 0, fb->len, pt);
}

/* copy file to buffer and append line break if needed */
void vi_open(const char *f, struct buf *fb)
{
	FILE *fp;
	char b[BUF_SIZE];
	char *ptr;
	size_t len = 0;

	fp = fopen(f, "r+");
	if (!fp)
		die("Error opening file");
	do {
		if ((len = fread(b, sizeof(b[0]), BUF_SIZE, fp))) {
			ptr = realloc(fb->b, sizeof(*fb->b) * (fb->len + len));
			if (ptr == NULL) {
				fclose(fp);
				die("realloc failed");
			}
			fb->b = ptr;
			memcpy(fb->b + fb->len, b, len);
			fb->len += len;
		}
	} while(len == BUF_SIZE);
	fclose(fp);
	if (fb->b != NULL) {
		ptr = realloc(fb->b, sizeof(*fb->b) * (fb->len + 1));
		if (ptr == NULL)
			die("realloc failed");
		fb->b = ptr;
		fb->b[fb->len] = '\0';
	}

	/* Append line break unless file is empty */
	if (fb->b != NULL && fb->b[fb->len - 1] != '\n') {
		fb->b[fb->len++] = '\n';
		ptr = realloc(fb->b, sizeof(*fb->b) * (fb->len + 1));
		if (ptr == NULL)
			die("realloc failed");
		fb->b = ptr;
		fb->b[fb->len] = '\0';
	}

	/* Count lines unless file is empty */
	if (fb->b != NULL) {
		ptr = fb->b;
		while ((ptr = strchr(ptr, '\n')) != NULL) {
			++fb->lines;
			++ptr;
		}
	}
}

/* get pointer to first char of line n */
char *vi_getline(const struct buf *fb, size_t n) {
	char *b = fb->b;
	for (size_t i = 1; i < n && i < fb->lines; ++i)
		b = strchr(b, '\n') + 1;
	return b;
}
