CC = gcc
CFLAGS = -Wall -Wextra -pedantic

.PHONY: all clean

all: main

main: main.o 
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Setting Capabilities"; sudo setcap cap_net_admin,cap_net_raw=eip $@

%.0: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  main *.o