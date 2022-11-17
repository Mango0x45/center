.POSIX:

CFLAGS   = -Wall -Wextra -Wpedantic -Werror \
	   -O3 -march=native -mtune=native  \
	   -pipe
PREFIX   = /usr
DPREFIX  = ${DESTDIR}${PREFIX}
MANDIR   = ${DPREFIX}/share/man
ZCOMPDIR = ${DPREFIX}/share/zsh/functions/Completion/Unix

target = center

all: ${target}
${target}: ${target}.c

clean:
	rm -f ${target}

install:
	mkdir -p ${DPREFIX}/bin ${MANDIR}/man1 ${ZCOMPDIR}
	cp ${target} ${DPREFIX}/bin
	cp ${target}.1 ${MANDIR}/man1
	cp completions/_${target} ${ZCOMPDIR}
