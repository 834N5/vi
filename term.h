#ifndef TERM_H
#define TERM_H

void term_init();
void term_getwinsize(unsigned short *row, unsigned short *col);
char process_keypress();

#endif
