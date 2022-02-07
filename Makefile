.POSIX:

CFLAGS = -Ofast -march=native -mtune=native -pipe
PREFIX = /usr

all: center
center: center.c

clean:
	rm -f center

install:
	mkdir -p ${PREFIX}/bin ${PREFIX}/share/man/man1
	cp center ${PREFIX}/bin
	cp center.1 ${PREFIX}/share/man/man1
