#ifndef TEXT_VIEW
#define TEXT_VIEW
#include "print.h"

int editorRowCxToRx(erow *row, int cx);

void editorUpdateRow(erow *row);

void editorOpen(char *filename);

void editorRowInsertChar(erow *row, int at, int c);

void editorInsertChar(int c);

void editorSave();

char *editorRowsToString(int *buflen);

void editorFreeRow(erow *row);

void editorDelRow(int at);

void editorRowDelChar(erow *row, int at);

void editorDelChar();

void editorInsertNewline();

#endif
