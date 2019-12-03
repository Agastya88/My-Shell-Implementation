CFLAGS=-g -Wall -pedantic

mysh: mysh.c
	gcc $(CFLAGS) -o mysh mysh.c

.PHONY: clean
clean:
	rm -f *.o $(PROGS)
