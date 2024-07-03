#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "vi.h"

/* TODO:
 * rename pointer variables with appropriate names
 * pcs -> pcs_ptr or pcs_p
 */

/* add piece to piece table */
void pt_add_piece(char buf, size_t start, size_t len, struct piece_table *pt)
{
	struct piece *ptr;

	ptr = realloc(pt->pcs, sizeof(*pt->pcs) * ++pt->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	pt->pcs = ptr;

	ptr = pt->pcs + pt->num_pcs - 1;
	ptr->buf = buf;
	ptr->start = start;
	ptr->len = len;
}

/* insert latest piece to latest operation in a specified position */
void op_insert_piece(struct piece_table *pt, size_t pos)
{
	struct operation *op_ptr;
	size_t *ptr;

	op_ptr = pt->ops + pt->num_ops - 1;
	ptr = op_ptr->pcs;

	ptr = realloc(ptr, sizeof(*(ptr)) * ++op_ptr->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	op_ptr->pcs = ptr;

	if (pos != op_ptr->num_pcs - 1) {
		memmove(
			ptr + pos + 1,
			ptr + pos,
			(op_ptr->num_pcs - pos - 1) * sizeof(*(op_ptr->pcs))
		);
	}

	*(op_ptr->pcs + pos) = pt->num_pcs - 1;
	op_ptr->len += (pt->pcs + pt->num_pcs - 1)->len;
}

/* insert text in a specified position in the piece table */
void pt_insert(char *b, size_t pos, struct buf *ab, struct piece_table *pt)
{
	int split = 0;
	size_t len = strlen(b);
	size_t table = 0;
	void *ptr;
	struct operation *op_ptr;

	ptr = realloc(ab->b, sizeof(ab->b) * (ab->len + len));
	if (ptr == NULL)
		die("realloc failed");
	ab->b = ptr;
	memcpy(ab->b + ab->len, b, len);
	ab->len += len;

	/* find position in the table to split */
	while (table < pt->num_table && pos != 0) {
		op_ptr = pt->ops + *(pt->table + table);
		if (op_ptr->len > pos) {
			split = 1;
			/*
			 * TODO:
			 * copy new operation from old op
			 * set del
			 * add new pieces to op
			 * replace table
			 */
			break;
		} else {
			pos -= op_ptr->len;
		}
		++table;
	}

	if (split) {
		puts("split!!!");
	} else if (!split) {
		op_ptr = realloc(pt->ops, sizeof(*pt->ops) * ++pt->num_ops);
		if (op_ptr == NULL)
			die("realloc failed");
		pt->ops = op_ptr;

		op_ptr = pt->ops + pt->num_ops - 1;
		op_ptr->pcs = NULL;
		op_ptr->num_pcs = 0;
		op_ptr->del = NULL;
		op_ptr->num_del = 0;
		op_ptr->len = 0;
		pt_add_piece('a', ab->len - len, len, pt);
		op_insert_piece(pt, 0);

		ptr = realloc(pt->table, sizeof(*pt->table) * ++pt->num_table);
		if (ptr == NULL)
			die("realloc failed");
		pt->table = ptr;
		memmove(
			pt->table + table + 1,
			pt->table + table,
			(pt->num_table - table - 1) * sizeof(*(pt->table))
		);
		*(pt->table + table) = pt->num_ops - 1;

		pt->len += len;

		/* implement undo later */
		//pt->undo = 0;
	}
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
	pt->ops->len = 0;
	pt_add_piece('f', 0, fb->len, pt);
	op_insert_piece(pt, 0);

	pt->len = fb->len;
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

/* not functional yet */
/* get pointer to first char of line n */
char *vi_getline(const struct buf *fb, size_t n) {
	char *b = fb->b;
	for (size_t i = 1; i < n && i < fb->lines; ++i)
		b = strchr(b, '\n') + 1;
	return b;
}
