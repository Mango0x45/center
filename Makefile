.POSIX:

CFLAGS = -Ofast -march=native -mtune=native -pipe -Wall -Wextra -Werror -pedantic
PREFIX = /usr

target = center

all: ${target}
${target}: ${target}.c

clean:
	rm -f ${target}

install:
	mkdir -p ${PREFIX}/bin ${PREFIX}/share/man/man1
	cp ${target} ${PREFIX}/bin
	cp ${target}.1 ${PREFIX}/share/man/man1
