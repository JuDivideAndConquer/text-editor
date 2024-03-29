#include "keyinput.h"
#include "print.h"
#include "view_text.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>

#define KILO_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

int getCursorPosition(int *rows ,int *cols)
{
	char buf[32];
	unsigned int i =0;
	if(write(STDOUT_FILENO, "\x1b[6n",4)!=4)return -1;

	while (i < sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	return 0;
}

int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
    	if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    	return getCursorPosition(rows, cols);
    }
	else
	{
    	*cols = ws.ws_col;
    	*rows = ws.ws_row;
    	return 0;
  	}
}

void initEditor()
{
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	E.dirty = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows-=2;
}

void editorScroll()
{
	E.rx = 0;
	if (E.cy < E.numrows)
	{
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
	}

	if (E.cy < E.rowoff)
	{
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows)
	{
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloff)
	{
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + E.screencols)
	{
		E.coloff = E.rx - E.screencols + 1;
	}
}

void editorDrawRows(struct abuf*ab)
{
	int y;
	for(y=0;y<E.screenrows;y++)
	{
		int filerow = y+ E.rowoff;
		if (filerow >= E.numrows)
		{
			if (E.numrows == 0 && y == E.screenrows / 3)
			{
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),"Syatem Lab text Editor by Irin and Sadrul");
				if (welcomelen > E.screencols) welcomelen = E.screencols;
				int padding = (E.screencols - welcomelen) / 2;
	      		if (padding)
	      		{
	        		abAppend(ab,"~", 1);
	        		padding--;
	      		}
	      		while(padding--)abAppend(ab," ",1);
				abAppend(ab, welcome, welcomelen);
			}
			else
			{
				abAppend(ab,"~", 1);
			}
			abAppend(ab, "\x1b[K", 3);
			if (y < E.screenrows - 1)
			{
				abAppend(ab, "\r\n", 2);
			}
		}
		else
		{
			int len = E.row[filerow].rsize - E.coloff;
			if (len < 0) len = 0;
      		if (len > E.screencols) len = E.screencols;
      		abAppend(ab, &E.row[filerow].render[E.coloff], len);
		}
		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}
}

void editorDrawStatusBar(struct abuf *ab)
{
	abAppend(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",E.filename ? E.filename : "[No Name]", E.numrows,E.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",E.cy + 1, E.numrows);
	if (len > E.screencols) len = E.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols)
	{
		if (E.screencols - len == rlen) 
		{
			abAppend(ab, rstatus, rlen);
			break;
		}
		else
		{
			abAppend(ab, " ", 1);
			len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
}

void editorDrawMessageBar(struct abuf *ab)
{
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols) msglen = E.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}


void editorRefreshScreen()
{
	editorScroll();
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	//abAppend(&ab, "\x1b[2J", 4);
	abAppend(&ab, "\x1b[H", 3);
	editorDrawRows(&ab);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,(E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));
	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

int editorReadKey()
{
	int nread;
	char c;
	while((nread =read(STDIN_FILENO,&c,1))!=1)
	{
		if(nread == -1 && errno !=EAGAIN)die("read");
	}
	if (c == '\x1b')
	{
	    char seq[3];
	    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
	    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
	    if (seq[0] == '[')
	    {
	    	if (seq[1] >= '0' && seq[1] <= '9')
	    	{
        		if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        		if (seq[2] == '~')
        		{
          			switch (seq[1])
          			{
          				case '1': return HOME_KEY;
          				case '3': return DEL_KEY;
            			case '4': return END_KEY;
            			case '5': return PAGE_UP;
            			case '6': return PAGE_DOWN;
            			case '7': return HOME_KEY;
            			case '8': return END_KEY;
            		}
            	}
            }
            else
            {
	    		switch (seq[1])
	    		{
		    		case 'A': return ARROW_UP;
	        		case 'B': return ARROW_DOWN;
	        		case 'C': return ARROW_RIGHT;
	        		case 'D': return ARROW_LEFT;
	        		case 'H': return HOME_KEY;
          			case 'F': return END_KEY;
	    		}
	    	}
	    }
	    return '\x1b';
	}
	else
	{
		return c;
	}
}

char *editorPrompt(char *prompt)
{
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	size_t buflen = 0;
	buf[0] = '\0';
	while (1)
	{
    	editorSetStatusMessage(prompt, buf);
    	editorRefreshScreen();
    	int c = editorReadKey();
    	if (c == '\r')
    	{
      		if (buflen != 0)
      		{
        		editorSetStatusMessage("");
        		return buf;
      		}
    	}
    	else if (!iscntrl(c) && c < 128)
    	{
    		if (buflen == bufsize - 1)
    		{
        		bufsize *= 2;
        		buf = realloc(buf, bufsize);
      		}
      		buf[buflen++] = c;
      		buf[buflen] = '\0';
    	}
	}
}
void editorMoveCursor(int key)
{
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	switch (key)
	{
    	case ARROW_LEFT:
	    	if (E.cx!=0)
	    	{
	    		E.cx--;
	    	}
	    	else if (E.cy > 0)
	    	{
	    		E.cy--;
	    		E.cx = E.row[E.cy].size;
	    	}
    		break;
    	case ARROW_RIGHT:
    		if (row && E.cx < row->size)
	    	{
	    		E.cx++;
	    	}
	    	else if (E.cy<E.numrows)
	    	{
	    		E.cy++;
        		E.cx = 0;
	    	}
    		break;
    	case ARROW_UP:
    		if (E.cy!=0)
	    	{
	    		E.cy--;
	    	}
    		break;
		case ARROW_DOWN:
			if (E.cy<E.numrows)
	    	{
	    		E.cy++;
	    	}
			break;
	}
	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen)
	{
		E.cx = rowlen;
	}
}
void editorProcessKeyPress()
{
	static int quit_times = KILO_QUIT_TIMES;
	int c =editorReadKey();
	int times;
	switch (c)
	{
		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0)
			{
        		editorSetStatusMessage("WARNING!!! File has unsaved changes. "
         		"Press Ctrl-Q %d more times to quit.", quit_times);
        		quit_times--;
        		return;
      		}
			write(STDOUT_FILENO, "\x1b[2J", 4);
    		write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		case '\r':
      		editorInsertNewline();
      		break;
      	case BACKSPACE:
    	case CTRL_KEY('h'):
    	case DEL_KEY:
      		if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      		editorDelChar();
      		break;
      	case CTRL_KEY('l'):
    	case '\x1b':
      		break;
      	case CTRL_KEY('s'):
      		editorSave();
      		break;
		case ARROW_UP:
    	case ARROW_DOWN:
    	case ARROW_LEFT:
    	case ARROW_RIGHT:
    		editorMoveCursor(c);
      		break;
      	case PAGE_UP:
    	case PAGE_DOWN:
    		if (c == PAGE_UP)
    		{
    			E.cy = E.rowoff;
    		}
    		else if (c == PAGE_DOWN)
    		{
    			E.cy = E.rowoff + E.screenrows - 1;
    			if (E.cy > E.numrows) E.cy = E.numrows;
    		}
    		times = E.screenrows;
    		while (times--)
    			editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    		break;
    	case HOME_KEY:
    		E.cx = 0;
    		E.cy = 0;
    		break;
    	case END_KEY:
    		if (E.cy < E.numrows)
        		E.cx = E.row[E.cy].size;
    		break;
    	default:
      		editorInsertChar(c);
      		break;
	}
	quit_times = KILO_QUIT_TIMES;
}

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}
void abFree(struct abuf *ab)
{
	free(ab->b);
}
