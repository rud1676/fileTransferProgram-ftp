CC =gcc
CFLAGS = -o

all: cli srv

srv : srv.c
	$(CC) $(CFLAGS) $@ $< -g
cli : cli.c
	$(CC) $(CFLAGS) $@ $< -g



