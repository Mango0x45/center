.POSIX:

CFLAGS = -Ofast -march=native -mtune=native -pipe -Wall -Wextra -Werror -pedantic
PREFIX = /usr
DPREFIX = ${DESTDIR}${PREFIX}

target = center

all: ${target}
${target}: ${target}.c

clean:
	rm -f ${target}

install:
	mkdir -p ${DPREFIX}/bin ${DPREFIX}/share/man/man1
	cp ${target} ${DPREFIX}/bin
	cp ${target}.1 ${DPREFIX}/share/man/man1
