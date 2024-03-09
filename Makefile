CC = gcc
CFLAGS = -Wall -Wextra -pedantic

.PHONY: all clean

all: main

server: main.o 
	$(CC) $(CFLAGS) -o $@ $^

%.0: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

setcap:
	sudo setcap cap_net_admin,cap_net_raw=eip $(main)

clean:
	rm -f  main *.0