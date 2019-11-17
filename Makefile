editor : 	editor.o print.o keyinput.o view_text.o
	gcc -o editor editor.o print.o keyinput.o view_text.o
print.o : 	print.c 
	gcc -c print.c
keyinput.o : keyinput.c
	gcc -c keyinput.c
view_text.o : view_text.c
	gcc -c view_text.c
editor.o :	editor.c
	gcc -c editor.c
clean : 
	rm editor print.o editor.o keyinput.o view_text.o
