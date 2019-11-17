#include "print.h"
#include "keyinput.h"
#include "view_text.h"
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>

#define _BSD_SOURCE
#define _GNU_SOURCE


int main(int argc,char *argv[])
{
    enableRawMode();
    initEditor();
    if (argc >= 2)
    {
        editorOpen(argv[1]);
    }
   	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");
    while(1)
    {
        editorRefreshScreen();
        editorProcessKeyPress();
    }
    return 0;
}