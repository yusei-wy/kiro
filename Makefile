CFLAGS=-std=c11 -g -static

kiro: kiro.c

clean:
	rm -f kiro *.o *~ tmp*

.PHONY: test clean
