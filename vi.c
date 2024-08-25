#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "term.h"

struct buf edit_buf = {NULL, 0};
struct piece_table pt = {NULL};
size_t cursor_pos = 0;
int cursor_x = 1;
int cursor_y = 1;
unsigned short rows = 24;
unsigned short cols = 80;

void free_all()
{
	free(edit_buf.b);
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

void draw_screen(size_t start, size_t lines)
{
	struct buf draw_buf = {NULL, 0};
	char *ptr;

	if (lines == 0)
		lines = 1;

	for (size_t i = 0; i < pt.num_table && lines != 0; ++i) {
		struct operation *op = pt.ops + *(pt.table + i);
		if (start != 0 && start > op->lines) {
			start -= op->lines;
			continue;
		}
		for (size_t j = 0; j < op->num_pcs && lines != 0; ++j) {
			size_t len;
			char *pos;
			struct piece *pc = pt.pcs + *(op->pcs + j);

			if (start > pc->lines) {
				start -= pc->lines;
				continue;
			}

			pos = edit_buf.b + pc->start;
			while (start != 0) {
				pos = memchr(pos, '\n', sizeof(*pos) * pc->len);
				++pos;
				--start;
			}
			while (pos < edit_buf.b + pc->start + pc->len) {
				len = (edit_buf.b + pc->start + pc->len) - pos;
				if (len > cols)
					len = cols + 1;
				ptr = memchr(pos, '\n', sizeof(*pos) * len);
				--len;
				if (ptr != NULL)
					len = ptr + 1 - pos;

				draw_buf.len += len;
				if (pos[len - 1] == '\n') {
					draw_buf.len += 3;
				}
				ptr = realloc(draw_buf.b,
					sizeof(*draw_buf.b) * draw_buf.len
				);
				if (ptr == NULL)
					die("realloc failed");
				draw_buf.b = ptr;

				ptr = draw_buf.b + draw_buf.len - len;
				if (pos[len - 1] == '\n') {
					ptr -= 3;
				}

				memcpy(ptr, pos, sizeof(*ptr) * len);
				if (pos[len - 1] == '\n') {
					memcpy(
						ptr + len - 1, "\x1b[K\n",
						sizeof(*ptr) * 4
					);
				}

				pos += len;
				if (--lines == 0)
					break;
			}
		}
	}
	if (lines != 0) {
		draw_buf.len += lines * 5 - 1;
		ptr = realloc(draw_buf.b, sizeof(*draw_buf.b) * draw_buf.len);
		if (ptr == NULL)
			die("realloc failed");
		draw_buf.b = ptr;
		ptr = draw_buf.b + (draw_buf.len - (lines * 5 - 1));
		for (size_t i = 0; i < lines - 1; ++i) {
			memcpy(ptr + i*5, "~\x1b[K\n", sizeof(*ptr) * 5);
		}
		memcpy(ptr + (lines - 1) * 5, "~\x1b[K", sizeof(*ptr) * 4);
	}
	fwrite(draw_buf.b, sizeof(*draw_buf.b), draw_buf.len, stdout);
	fflush(stdout);
	free(draw_buf.b);
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

		/* TODO: change to something like this */
		/*
		draw_screen(0, rows - 1);
		fwrite(buf, sizeof(*buf), buf_len, stdout);
		draw_screen(0, rows - 1);
		*/

		fwrite("\x1b[2J" "\x1b[H", sizeof(char), 7, stdout);
		while (curr_op < pt.num_table && temp == 0) {
			op = pt.ops + *(pt.table + curr_op++);
			curr_pc = 0;
			while (curr_pc < op->num_pcs && temp == 0) {
				pc = pt.pcs + *(op->pcs + curr_pc++);
				ptr = edit_buf.b + pc->start;
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
					ptr = edit_buf.b + pc->start;
					fwrite(ptr, sizeof(*ptr), pc->len, stdout);
				}
				curr_pc = 0;
			}
		}
		printf("\x1b[%d;%dH", cursor_y, cursor_x);
		fflush(stdout);
		temp = 0;
	}

	if (buf_len != 0) {
		buf[buf_pos] = '\0';
		pt_insert(buf, cursor_pos, &edit_buf, &pt);
	}

	free(buf);
	return buf_pos;
}

int main(int argc, char *argv[])
{
	size_t testingshit = 0;
	char c;
	atexit(free_all);
	if (argc < 2)
		exit(EXIT_SUCCESS);
	vi_open(argv[1], &edit_buf);
	pt_init(&edit_buf, &pt);
	term_init();
	term_getwinsize(&rows, &cols);

	fwrite("\x1b[H", sizeof(char), 3, stdout);
	draw_screen(0, rows - 1);
	for (;;) {
		printf("\x1b[%d;%dH", cursor_y, cursor_x);
		fflush(stdout);
		c = process_keypress();
		switch (c) {
		case 'q':
			exit(EXIT_SUCCESS);
			break;
		case 'i':
			cursor_pos += input_mode();
			draw_screen(testingshit, rows - 1);
			break;
		case 'k':
			if (testingshit != 0)
				draw_screen(--testingshit, rows - 1);
			break;
		case 'j':
			if (testingshit < pt.lines - rows + 1)
				draw_screen(++testingshit, rows - 1);
			break;
		default:
			break;
		}
	}

	exit(EXIT_SUCCESS);
}
