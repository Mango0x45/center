/*
 * BSD Zero Clause License
 *
 * Copyright (c) 2022 Thomas Voss
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define POSIXLY_CORRECT
#define _POSIX_C_SOURCE 200809L

#include <sys/ioctl.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ESC 033

static void center(FILE *);
static int cols(void);
static int utf8len(const char *);
static int noesclen(const char *);
static int matchesc(const char *);

extern int optind;
extern char *optarg;

int rval;
long width;
int (*lenfunc)(const char *) = noesclen;

int
main(int argc, char **argv)
{
	int opt;
	char *endptr;

	while ((opt = getopt(argc, argv, ":elsw:")) != -1) {
		switch (opt) {
		case 'e':
			lenfunc = utf8len;
			break;
		case 'w':
			width = strtol(optarg, &endptr, 0);
			if (*optarg == '\0' || *endptr != '\0')
				errx(EXIT_FAILURE, "Invalid integer '%s'", optarg);
			if (width <= 0)
				errx(EXIT_FAILURE, "Width must be >0");
			if (errno == ERANGE || width > INT_MAX)
				warnx("Potential overflow of given width");
			break;
		default:
			fprintf(stderr, "Usage: %s [-e] [-w width] [file ...]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!width && (width = cols()) == -1)
		errx(EXIT_FAILURE, "Unable to determine output width");

	argc -= optind;
	argv += optind;

	if (argc == 0)
		center(stdin);
	else do {
		if (strcmp(*argv, "-") == 0)
			center(stdin);
		else {
			FILE *fp;
			if ((fp = fopen(*argv, "r")) == NULL) {
				warn("fopen");
				rval = EXIT_FAILURE;
			} else {
				center(fp);
				fclose(fp);
			}
		}
	} while (*++argv);

	return rval;
}

/* Write the contents of the file pointed to by `fp' center aligned to the standard output */
void
center(FILE *fp)
{
	char *line = NULL;
	size_t bs = 0;

	while (getline(&line, &bs, fp) != -1) {
		int len, tabs = 0;
		char *match = line;

		while (strchr(match, '\t')) {
			match++;
			tabs++;
		}

		len = lenfunc(line) + tabs * 8 - tabs;
		for (int i = (width - len) / 2; i; i--)
			putchar(' ');
		fputs(line, stdout);
	}
	if (ferror(fp)) {
		warn("getline");
		rval = EXIT_FAILURE;
	}
}

/* Return the column width of the output device if it is a TTY. If it is not a TTY then return -1 */
int
cols(void)
{
	struct winsize w;

	if (!isatty(STDOUT_FILENO))
		return -1;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
		err(EXIT_FAILURE, "ioctl");

	return w.ws_col;
}

/* Return the length of the UTF-8 encoded string `s' in characters, not bytes */
int
utf8len(const char *s)
{
	int l = 0;

	while (*s)
		l += (*s++ & 0xC0) != 0x80;

	if (l > 0 && s[-1] == '\n')
		l--;

	return l;
}

/* Return the length of the UTF-8 encoded string `s' in characters, not bytes. Additionally this
 * function ignores ANSI color escape sequences when computing the length.
 */
int
noesclen(const char *s)
{
	int off = 0;
	const char *d = s;

	while ((d = strchr(d, ESC)) != NULL)
		off += matchesc(++d);
	
	return utf8len(s) + off;
}

/* Return the length of the ANSI color escape sequence at the beginning of the string `s'. If no
 * escape sequence is matched then 0 is returned. The local variable `c' is initialized to 3 in order
 * to account for the leading ESC, the following `[', and the trailing `m'.
 */
int
matchesc(const char *s)
{
	int c = 3;
	
	if (*s++ != '[')
		return 0;
	while (isdigit(*s) || *s == ';') {
		c++;
		s++;
	}
	
	return *s == 'm' ? c : 0;
}
