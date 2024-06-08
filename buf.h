#ifndef BUF_H
#define BUF_H

#define BUF_SIZE 16384

struct fb {
	char *b;
	size_t len;
	size_t lines;
};

int vi_open(const char *f, struct fb *fb);

char *vi_getline(const struct fb *fb, size_t n);

#endif
