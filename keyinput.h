#ifndef KEY_PRESS
#define KEY_PRESS

#include<string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>

struct abuf
{
	char *b;
	int len;
};

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

#define ABUF_INIT {NULL, 0}

void editorRefreshScreen();

char *editorPrompt(char *prompt);

int editorReadKey();

void editorMoveCursor(int key);

void editorProcessKeyPress();

void editorDrawStatusBar(struct abuf *ab);

void editorDrawRows(struct abuf*ab);

int getWindowSize(int *rows, int *cols);

void initEditor();

void abAppend(struct abuf *ab, const char *s, int len);

void abFree(struct abuf *ab);

void editorSetStatusMessage(const char *fmt, ...);

void editorDrawMessageBar(struct abuf *ab);

#endif