#ifndef PRINT
#define PRINT

#include<termios.h>
#include<time.h>


/*erow stand for editor row and store line of text as a pointer to the 
dynamically allocated charecter data and length*/

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

struct editorConfig
{
	int cx,cy;//position of the cursior
	int rx;
	int rowoff;//to display each row in the file 
	int coloff;//to display each col in the file
	int screenrows;//number of rows inthe text editor
	int screencols;//number of col in the text editor
	struct termios orig_termios;
	int numrows;//row number we are currently editing
  	erow*row;//row buffer to save all the line in the file
  	char *filename;
  	char statusmsg[80];
  	time_t statusmsg_time;
  	int dirty;
};

struct editorConfig E;

void print_char(char c);

//void print_hw();

void die(const char *s);

void disableRawMode();

void enableRawMode();

#endif
