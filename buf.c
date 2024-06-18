#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "vi.h"

/* add piece to piece table */
size_t pt_add_piece(char b, size_t start, size_t len, struct piece_table *pt)
{
	struct piece *ptr = realloc(pt->pcs, sizeof(*pt->pcs) * ++pt->num_pcs);
	if (ptr == NULL)
		die("realloc failed");
	pt->pcs = ptr;
	pt->pcs->buf = b;
	pt->pcs->start = start;
	pt->pcs->len = len;
	return pt->num_pcs - 1;
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
	++pt->num_table;

	if ((ptr = malloc(sizeof(*pt->ops))) == NULL)
		die("malloc failed");
	pt->ops = ptr;
	++pt->num_ops;

	if ((ptr = malloc(sizeof(*pt->ops->append))) == NULL)
		die("malloc failed");
	pt->ops->append = ptr;
	++pt->ops->num_append;

	*pt->table = 0;
	*pt->ops->append = pt_add_piece('f', 0, fb->len, pt);
	pt->ops->del = NULL;
	pt->ops->num_del = 0;

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
