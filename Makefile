CC 			= gcc
CFLAGS	= -g -lncurses
RM 			= rm -f

all: main

main: main.c
	$(CC) $(CFLAGS) -o snake_tui main.c

clean veryclean:
	$(RM) snake_tui
