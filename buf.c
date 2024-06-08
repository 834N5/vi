#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"

int vi_open(const char *f, struct fb *fb)
{
	FILE *fp;
	char b[BUF_SIZE];
	size_t len = 0;

	if ((fp = fopen(f, "r")) == NULL)
		return 1;
	do {
		if ((len = fread(b, sizeof(b[0]), BUF_SIZE, fp))) {
			fb->len += len;
			fb->b = realloc(fb->b, sizeof(*fb->b) * (fb->len));
			memcpy(fb->b + fb->len - len, b, len);
		}
	} while(len == BUF_SIZE);
	fclose(fp);
	fb->b = realloc(fb->b, sizeof(*fb->b) * (fb->len + 1));
	fb->b[fb->len] = '\0';

	/* Append line break unless file is empty */
	if (fb->len && fb->b[fb->len - 1] != '\n') {
		fb->b[fb->len++] = '\n';
		fb->b = realloc(fb->b, sizeof(*fb->b) * (fb->len + 1));
		fb->b[fb->len] = '\0';
	}
	return 0;
}
