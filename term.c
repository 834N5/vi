#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "vi.h"

struct termios term_orig;

void term_reset()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_orig) == -1)
		die("tcsetattr");
	fwrite("\x1b[2J" "\x1b[H", sizeof(char), 7, stdout);
}

void term_getwinsize(unsigned short *row, unsigned short *col)
{
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	if (ws.ws_col == 0 || ws.ws_row == 0) {
		*row = 24;
		*col = 80;
	} else {
		*row = ws.ws_row;
		*col = ws.ws_col;
	}
}

void term_init()
{
	if (tcgetattr(STDIN_FILENO, &term_orig) == -1)
		die("tcgetattr");
	atexit(term_reset);

	struct termios term_vi = term_orig;
	term_vi.c_iflag &= ~(ICRNL);
	term_vi.c_lflag &= ~(ECHO | ICANON | ISIG);
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_vi) == -1)
		die("tcsetattr");
}

char process_keypress()
{
	char c;
	int bytes;
	while ((bytes = read(STDIN_FILENO, &c, 1)) != 1)
		if (bytes == -1)
			die("read");
	return c;
}
