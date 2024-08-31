#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "term.h"

struct buf edit_buf = {NULL, 0};
struct buf display_buf = {NULL, 0};
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

void update_display_buffer(size_t start, size_t lines)
{
	char *ptr;
	size_t display_len = 0;

	free(display_buf.b);
	display_buf.b = NULL;
	display_buf.len = 0;

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
			struct piece *pc = pt.pcs + *(op->pcs + j);
			char *start_pos;
			char *pos = edit_buf.b + pc->start;
			char *end = edit_buf.b + pc->start + pc->len;

			if (start > pc->lines) {
				start -= pc->lines;
				continue;
			}

			while (start != 0) {
				pos = memchr(pos, '\n', sizeof(*pos) * pc->len);
				++pos;
				--start;
			}
			start_pos = pos;
			while (pos < end) {
				char *ht_ptr, *lf_ptr;
				size_t ht_spaces = 0;
				size_t left = end - pos;
				ht_ptr = memchr(pos, '\t', sizeof(*pos) * left);
				lf_ptr = memchr(pos, '\n', sizeof(*pos) * left);
				if (ht_ptr != NULL && lf_ptr == NULL) {
					ptr = ht_ptr;
				} else if (ht_ptr == NULL && lf_ptr != NULL) {
					ptr = lf_ptr;
				} else if (ht_ptr != NULL && lf_ptr != NULL) {
					if (ht_ptr < lf_ptr)
						ptr = ht_ptr;
					else
						ptr = lf_ptr;
				} else {
					ptr = end - 1;
				}

				display_len += ptr - pos + 1;
				if (*ptr == '\t')
					ht_spaces = 8 - display_len % 8;
				display_len += ht_spaces;
				if (*ptr == '\n')
					--display_len;

				if (lines * cols <= display_len) {
					size_t rm_len = 0;
					size_t max_size = lines * cols;
					if (*ptr == '\t' || *ptr == '\n')
						--ptr;
					display_len -= ht_spaces;
					if (display_len > max_size)
						rm_len = display_len - max_size;
					display_len -= rm_len;
					pos = ptr - rm_len + 1;
					break;
				} if (*ptr == '\n') {
					lines -= (display_len + cols-1) / cols;
					if (display_len == 0)
						--lines;
					display_len = 0;
				}
				pos = ptr + 1;
			}
			lines -= (display_len + cols-1) / cols;
			display_len = 0;
			len = pos - start_pos;
			display_buf.len += len;
			ptr = realloc(
				display_buf.b,
				sizeof(*display_buf.b) * display_buf.len
			);
			if (ptr == NULL)
				die("realloc failed");
			display_buf.b = ptr;
			ptr = display_buf.b + display_buf.len - len;
			memcpy(ptr, start_pos, sizeof(*ptr) * len);
		}
	}
}

void draw_display_buffer()
{
	size_t lines = rows;
	struct buf draw_buf = {NULL, 0};
	char *ptr;
	char *pos;

	if (lines != 1)
		--lines;

	pos = display_buf.b;
	for (size_t len = 0, left = display_buf.len; left > 0 ; left -= len) {
		size_t draw_len;
		size_t tabs = 0;
		size_t tab_spaces = 0;

		ptr = memchr(pos, '\n', sizeof(*pos) * left);
		if (ptr != NULL)
			len = ptr + 1 - pos;
		else
			len = left;

		ptr = pos;
		for (size_t left = len; left > 0; ++tabs) {
			ptr = memchr(ptr, '\t', sizeof(*ptr) * left);
			if (ptr != NULL) {
				size_t prev_len = ptr - pos + tab_spaces - tabs;
				tab_spaces += 8 - prev_len % 8;
				left = len - (++ptr - pos);
			} else {
				break;
			}
		}
		draw_len = len + tab_spaces - tabs;

		draw_buf.len += draw_len + 3;
		ptr = realloc(draw_buf.b, sizeof(*draw_buf.b) * draw_buf.len);
		if (ptr == NULL)
			die("realloc failed");
		draw_buf.b = ptr;

		for (size_t len = 0, left = draw_len; left > 0; ) {
			size_t cp_len = 0;
			size_t spaces;
			ptr = memchr(pos, '\t', sizeof(*ptr) * left);

			if (ptr != NULL) {
				cp_len = ptr - pos;
				len += cp_len;
				spaces = 8 - len % 8;
				ptr = draw_buf.b + draw_buf.len - (left + 3);
				memcpy(ptr, pos, sizeof(*ptr) * cp_len);
				for (size_t i = 0; i < spaces; ++i)
					*(ptr + cp_len + i) = ' ';
				pos += cp_len + 1;
				left -= cp_len + spaces;
				len += spaces;
			} else {
				ptr = draw_buf.b + draw_buf.len - (left + 3);
				memcpy(ptr, pos, sizeof(*ptr) * left);
				pos += left;
				left = 0;
			}
		}
		ptr = draw_buf.b + draw_buf.len - 3;
		if (*(ptr - 1) == '\n' && (draw_len - 1) % cols != 0) {
			memcpy(ptr - 1, "\x1b[K\n", sizeof(*ptr) * 4);
			lines -= (draw_len - 1 + cols - 1) / cols;
			if (len == 1)
				--lines;
		} else if (draw_len % cols != 0) {
			memcpy(ptr, "\x1b[K", sizeof(*ptr) * 3);
			lines -= (draw_len + cols - 1) / cols;
		} else {
			draw_buf.len -= 3;
			ptr = realloc(
				draw_buf.b,
				sizeof(*draw_buf.b) * draw_buf.len
			);
			lines -= (draw_len + cols - 1) / cols;
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
			ptr = realloc(buf, sizeof(*buf) * (++buf_len + 2));
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
		if (cursor_pos == pt.len) {
			buf[buf_pos] = '\n';
			buf[buf_pos + 1] = '\0';
		} else {
			buf[buf_pos] = '\0';
		}
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
	update_display_buffer(0, rows - 1);
	draw_display_buffer();
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
			fwrite("\x1b[H", sizeof(char), 3, stdout);
			update_display_buffer(testingshit, rows - 1);
			draw_display_buffer();
			break;
		case 'k':
			if (testingshit != 0) {
				fwrite("\x1b[H", sizeof(char), 3, stdout);
				update_display_buffer(--testingshit, rows - 1);
				draw_display_buffer();
			}
			break;
		case 'j':
			if (testingshit < pt.lines - 1) {
				fwrite("\x1b[H", sizeof(char), 3, stdout);
				update_display_buffer(++testingshit, rows - 1);
				draw_display_buffer();
			}
			break;
		default:
			break;
		}
	}

	exit(EXIT_SUCCESS);
}
