#include <stdio.h>
#include <stdlib.h>
#include "buf.h"
#include "term.h"

struct buf fb = {NULL, 0};
struct buf ab = {NULL, 0};
struct buf display_buf = {NULL, 0};
struct piece_table pt = {NULL};
size_t cursor_pos = 0;
int cursor_x = 1;
int cursor_y = 1;
unsigned short rows = 24;
unsigned short cols = 80;

void free_all()
{
	free(fb.b);
	free(ab.b);
	for (size_t i = 0; i < pt.num_ops; ++i) {
		free((pt.ops + i)->pcs);
		free((pt.ops + i)->del);
	}
	free(pt.pcs);
	free(pt.ops);
	free(pt.table);
}

void die(const char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

void print()
{
	char *ptr;
	fwrite("\x1b[2J" "\x1b[H", sizeof(char), 7, stdout);
	for (size_t i = 0; i < pt.num_table; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		for (size_t j = 0; j < op->num_pcs; ++j) {
			struct piece *pc = pt.pcs + *(op->pcs + j);
			if (pc->buf == 'f')
				ptr = fb.b + pc->start;
			else
				ptr = ab.b + pc->start;
			fwrite(ptr, sizeof(*ptr), pc->len, stdout);
		}
	}
	fflush(stdout);
}

size_t input_mode()
{
	char *ptr = NULL;
	char *buf = malloc(sizeof(char));
	*buf = '\0';
	size_t buf_pos = 0;
	size_t buf_len = 0;

	for (;;) {
		char c = process_keypress();
		size_t curr_op = 0;
		size_t curr_pc = 0;
		struct operation *op = NULL;
		struct piece *pc = NULL;
		size_t pos = cursor_pos;
		size_t temp = 0;

		if (c == 0x1B)
			break;
		if (c == 0x08 || c == 0x7F) {
			if (buf_pos != 0 && buf[buf_pos-1] != '\n') {
				--buf_pos;
				printf("\x1b[%d;%dH", cursor_y, cursor_x);
				fflush(stdout);
			}
		}
		if ((c >= 0x0 && c <= 0x1F) || c == 0x7F)
			if (c != '\t' && c != '\r' && c != '\n')
				continue;
		if (buf_pos == buf_len) {
			ptr = realloc(buf, sizeof(*buf) * (++buf_len + 1));
			if (ptr == NULL)
				die("realloc failed");
			buf = ptr;
		}
		buf[buf_pos++] = (c == '\r') ? '\n' : c;
		if (buf[buf_pos - 1] == '\n')
			buf_len = buf_pos;

		fwrite("\x1b[2J" "\x1b[H", sizeof(char), 7, stdout);
		while (curr_op < pt.num_table && temp == 0) {
			op = pt.ops + *(pt.table + curr_op++);
			curr_pc = 0;
			while (curr_pc < op->num_pcs && temp == 0) {
				pc = pt.pcs + *(op->pcs + curr_pc++);
				if (pc->buf == 'f')
					ptr = fb.b + pc->start;
				else
					ptr = ab.b + pc->start;
				if (pc->len >= pos && temp == 0) {
					fwrite(ptr, sizeof(*ptr), pos, stdout);
					temp = 1;
				} else {
					pos -= pc->len;
					fwrite(ptr, sizeof(*ptr), pc->len, stdout);
				}
			}
		}
		fwrite(buf, sizeof(*buf), buf_len, stdout);
		if (op != NULL) {
			fwrite(ptr + pos, sizeof(*ptr), pc->len - pos, stdout);
			--curr_op;
			while (curr_op < pt.num_table) {
				op = pt.ops + *(pt.table + curr_op++);
				while (curr_pc < op->num_pcs) {
					pc = pt.pcs + *(op->pcs + curr_pc++);
					if (pc->buf == 'f')
						ptr = fb.b + pc->start;
					else
						ptr = ab.b + pc->start;
					fwrite(ptr, sizeof(*ptr), pc->len, stdout);
				}
				curr_pc = 0;
			}
		}
		printf("\x1b[%d;%dH", cursor_y, cursor_x);
		fflush(stdout);
		temp = 0;
	}

	buf[buf_pos] = '\0';
	pt_insert(buf, cursor_pos, &fb, &ab, &pt);

	free(buf);
	return buf_pos;
}

int main(int argc, char *argv[])
{
	char c;
	atexit(free_all);
	if (argc < 2)
		exit(EXIT_SUCCESS);
	vi_open(argv[1], &fb);
	pt_init(&fb, &pt);
	term_init();
	term_getwinsize(&rows, &cols);

	/* temp testing */
	fwrite("\x1b[2J" "\x1b[H", sizeof(char), 7, stdout);
	for (;;) {
		print();
		//printf("\x1b[%d;%dH", cursor_y, cursor_x);
		printf("\x1b[%d;%dH", rows, cols);
		fflush(stdout);
		c = process_keypress();
		switch (c) {
		case 'q':
			exit(EXIT_SUCCESS);
			break;
		case 'i':
			cursor_pos += input_mode();
			break;
		default:
			break;
		}
	}
	/* temp testing ends */

	exit(EXIT_SUCCESS);
}
