.POSIX:

CFLAGS  = -Ofast -march=native -mtune=native -pipe -Wall -Wextra -Werror -pedantic
PREFIX  = /usr
DPREFIX = ${DESTDIR}${PREFIX}
MANDIR  = ${DPREFIX}/share/man

target = center

all: ${target}
${target}: ${target}.c

clean:
	rm -f ${target}

install:
	mkdir -p ${DPREFIX}/bin ${MANDIR}/man1
	cp ${target} ${DPREFIX}/bin
	cp ${target}.1 ${MANDIR}/man1
