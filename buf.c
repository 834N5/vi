#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "vi.h"

/* copy file to buffer and append line break if needed */
void vi_open(const char *f, struct buf *fb)
{
	FILE *fp;
	char b[BUF_SIZE];
	char *p;
	size_t len = 0;

	fp = fopen(f, "r+");
	if (!fp)
		die("Error opening file");
	do {
		if ((len = fread(b, sizeof(b[0]), BUF_SIZE, fp))) {
			p = realloc(fb->b, sizeof(*fb->b) * (fb->len + len));
			if (!p) {
				fclose(fp);
				die("realloc failed");
			}
			fb->b = p;
			memcpy(fb->b + fb->len, b, len);
			fb->len += len;
		}
	} while(len == BUF_SIZE);
	fclose(fp);
	p = realloc(fb->b, sizeof(*fb->b) * (fb->len + 1));
	if (!p)
		die("realloc failed");
	fb->b = p;
	fb->b[fb->len] = '\0';

	/* Append line break unless file is empty */
	if (fb->len && fb->b[fb->len - 1] != '\n') {
		p = realloc(fb->b, sizeof(*fb->b) * (fb->len + 2));
		if (!p)
			die("realloc failed");
		fb->b = p;
		fb->b[fb->len++] = '\n';
		fb->b[fb->len] = '\0';
	}

	for (char *c = fb->b; (c = strchr(c, '\n')) != NULL; ++fb->lines, ++c);
}

/* get pointer to first char of line n */
char *vi_getline(const struct buf *fb, size_t n) {
	char *b = fb->b;
	for (size_t i = 1; i < n && i < fb->lines; ++i)
		b = strchr(b, '\n') + 1;
	return b;
}
