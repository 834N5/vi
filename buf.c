#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "vi.h"

/* add piece to piece table */
void pt_add_piece(size_t start, size_t len, size_t lines, struct piece_table *pt)
{
	struct piece *ptr;

	ptr = realloc(pt->pcs, sizeof(*pt->pcs) * ++pt->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	pt->pcs = ptr;

	ptr = pt->pcs + pt->num_pcs - 1;
	ptr->start = start;
	ptr->len = len;
	ptr->lines = lines;

}

/* insert latest piece to latest operation in a specified position */
void op_insert_piece(struct piece_table *pt, size_t pos)
{
	struct operation *op_ptr;
	size_t *ptr;

	op_ptr = pt->ops + pt->num_ops - 1;
	ptr = op_ptr->pcs;

	ptr = realloc(ptr, sizeof(*op_ptr->pcs) * ++op_ptr->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	op_ptr->pcs = ptr;

	if (pos != op_ptr->num_pcs - 1) {
		memmove(
			ptr + pos + 1,
			ptr + pos,
			(op_ptr->num_pcs - pos - 1) * sizeof(*op_ptr->pcs)
		);
	}

	*(op_ptr->pcs + pos) = pt->num_pcs - 1;
	op_ptr->len += (pt->pcs + pt->num_pcs - 1)->len;
	op_ptr->lines += (pt->pcs + pt->num_pcs - 1)->lines;
}

/* insert text in a specified position in the piece table */
void pt_insert(char *b, size_t pos, struct buf *eb, struct piece_table *pt)
{
	int split = 0;
	size_t len = strlen(b);
	size_t table_split = 0;
	size_t op_split = 0;
	size_t pc_split = 0;
	size_t lines = 0;
	size_t lines_tmp = 0;
	void *ptr;
	char *char_ptr;
	struct operation *op_ptr;
	struct piece *pc_ptr;

	ptr = realloc(eb->b, sizeof(eb->b) * (eb->len + len));
	if (ptr == NULL)
		die("realloc failed");
	eb->b = ptr;
	memcpy(eb->b + eb->len, b, len);
	eb->len += len;

	char_ptr = b;
	while ((char_ptr = strchr(char_ptr, '\n')) != NULL) {
		++lines;
		++char_ptr;
	}

	/* find position in the table to split */
	while (table_split < pt->num_table && pos != 0) {
		op_ptr = pt->ops + *(pt->table + table_split);
		if (op_ptr->len > pos) {
			split = 1;
			break;
		} else {
			pos -= op_ptr->len;
		}
		++table_split;
	}
	if (pt->num_table != 0)
		op_split = *(pt->table + table_split);

 	/* find position in the operation to split */
	if (split == 1) {
		while (pc_split < op_ptr->num_pcs && pos != 0) {
			pc_ptr = pt->pcs + *(op_ptr->pcs + pc_split);
			if (pc_ptr->len > pos) {
				/* need to split a piece */
				split = 2;
				break;
			} else {
				pos -= pc_ptr->len;
			}
			++pc_split;
		}
	}


	op_ptr = realloc(pt->ops, sizeof(*pt->ops) * ++pt->num_ops);
	if (op_ptr == NULL)
		die("realloc failed");
	pt->ops = op_ptr;
	op_ptr = pt->ops + pt->num_ops - 1;

	/* split piece */
	if (split == 2) {
		op_ptr->num_pcs = (pt->ops + op_split)->num_pcs;
		--op_ptr->num_pcs;
		ptr = malloc(sizeof(op_ptr->pcs) * op_ptr->num_pcs);
		if (ptr == NULL)
			die("malloc failed");
		op_ptr->pcs = ptr;
		memcpy(
			op_ptr->pcs,
			(pt->ops + op_split)->pcs,
			(pc_split) * sizeof(*pt->ops->pcs)
		);
		memcpy(
			op_ptr->pcs + pc_split,
			(pt->ops + op_split)->pcs + pc_split + 1,
			(op_ptr->num_pcs - pc_split) * sizeof(*pt->ops->pcs)
		);

		ptr = malloc(sizeof(*op_ptr->del));
		if (ptr == NULL)
			die("malloc failed");
		op_ptr->del = ptr;
		*op_ptr->del = op_split;
		op_ptr->num_del = 1;

		op_ptr->len = (pt->ops + op_split)->len - pc_ptr->len;
		op_ptr->lines = (pt->ops + op_split)->lines - pc_ptr->lines;

		char_ptr = eb->b + pc_ptr->start;
		for (size_t i = 0; i < pc_ptr->lines; ++i) {
			char_ptr = strchr(char_ptr, '\n');
			if (char_ptr < (eb->b + pc_ptr->start + pos)) {
				++lines_tmp;
				++char_ptr;
			} else {
				break;
			}
		}

		pt_add_piece(
			pc_ptr->start + pos,
			pc_ptr->len - pos,
			pc_ptr->lines - lines_tmp,
			pt
		);
		op_insert_piece(pt, pc_split);
		pc_ptr = pt->pcs + *((pt->ops + op_split)->pcs + pc_split);

		pt_add_piece(eb->len - len, len, lines, pt);
		op_insert_piece(pt, pc_split);
		pc_ptr = pt->pcs + *((pt->ops + op_split)->pcs + pc_split);

		pt_add_piece(pc_ptr->start, pos, lines_tmp, pt);
		op_insert_piece(pt, pc_split);

		*(pt->table + table_split) = pt->num_ops - 1;
	}

	/* split operation without splitting piece */
	if (split == 1) {
		op_ptr->num_pcs = (pt->ops + op_split)->num_pcs;
		ptr = malloc(sizeof(op_ptr->pcs) * op_ptr->num_pcs);
		if (ptr == NULL)
			die("malloc failed");
		op_ptr->pcs = ptr;
		memcpy(
			op_ptr->pcs,
			(pt->ops + op_split)->pcs,
			(op_ptr->num_pcs) * sizeof(*pt->ops->pcs)
		);

		ptr = malloc(sizeof(*op_ptr->del));
		if (ptr == NULL)
			die("malloc failed");
		op_ptr->del = ptr;
		*op_ptr->del = op_split;
		op_ptr->num_del = 1;

		op_ptr->len = (pt->ops + op_split)->len;
		op_ptr->lines = (pt->ops + op_split)->lines;

		pt_add_piece(eb->len - len, len, lines, pt);
		op_insert_piece(pt, pc_split);
		*(pt->table + table_split) = pt->num_ops - 1;
	}

	/* no splitting required */
	if (split == 0) {
		op_ptr->pcs = NULL;
		op_ptr->num_pcs = 0;
		op_ptr->del = NULL;
		op_ptr->num_del = 0;
		op_ptr->len = 0;
		op_ptr->lines = 0;
		pt_add_piece(eb->len - len, len, lines, pt);
		op_insert_piece(pt, 0);

		ptr = realloc(pt->table, sizeof(*pt->table) * ++pt->num_table);
		if (ptr == NULL)
			die("realloc failed");
		pt->table = ptr;
		memmove(
			pt->table + table_split + 1,
			pt->table + table_split,
			(pt->num_table - table_split - 1) * sizeof(*pt->table)
		);
		*(pt->table + table_split) = pt->num_ops - 1;
	}

	pt->len += len;
	pt->lines += lines;
}

/* initialise piece table */
void pt_init(struct buf *eb, struct piece_table *pt)
{
	void *ptr;
	char *char_ptr;
	size_t lines = 0;

	if (eb->b == NULL)
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
	pt->ops->lines = 0;

	char_ptr = eb->b;
	while ((char_ptr = strchr(char_ptr, '\n')) != NULL) {
		++lines;
		++char_ptr;
	}

	pt_add_piece(0, eb->len, lines, pt);
	op_insert_piece(pt, 0);

	pt->len = eb->len;
	pt->lines = lines;
}

/* copy file to buffer and append line break if needed */
void vi_open(const char *f, struct buf *eb)
{
	FILE *fp;
	char b[BUF_SIZE];
	char *ptr;
	size_t len = 0;

	fp = fopen(f, "r+");
	if (!fp)
		die("Error opening file");
	do {
		if ((len = fread(b, sizeof(*b), BUF_SIZE, fp))) {
			ptr = realloc(eb->b, sizeof(*eb->b) * (eb->len + len));
			if (ptr == NULL) {
				fclose(fp);
				die("realloc failed");
			}
			eb->b = ptr;
			memcpy(eb->b + eb->len, b, len);
			eb->len += len;
		}
	} while(len == BUF_SIZE);
	fclose(fp);
	if (eb->b != NULL) {
		ptr = realloc(eb->b, sizeof(*eb->b) * (eb->len + 1));
		if (ptr == NULL)
			die("realloc failed");
		eb->b = ptr;
		eb->b[eb->len] = '\0';
	}

	/* Append line break unless file is empty */
	if (eb->b != NULL && eb->b[eb->len - 1] != '\n') {
		eb->b[eb->len++] = '\n';
		ptr = realloc(eb->b, sizeof(*eb->b) * (eb->len + 1));
		if (ptr == NULL)
			die("realloc failed");
		eb->b = ptr;
		eb->b[eb->len] = '\0';
	}
}

void vi_save(const char *f, struct piece_table *pt, struct buf *eb)
{
	FILE *fp;
	fp = fopen(f, "w");
	for (size_t i = 0; i < pt->num_table; ++i) {
		struct operation *op = pt->ops + *(pt->table + i);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt->pcs + *(op->pcs + j);
			fwrite(eb->b + pc->start, sizeof(*eb->b), pc->len, fp);
		}
	}
	fclose(fp);
}
