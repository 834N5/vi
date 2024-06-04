#ifndef BUF_H
#define BUF_H

#define BUF_SIZE 16384

struct fb {
	char *b;
	size_t len;
};

int vi_open(const char *f, struct fb *fb);

#endif
