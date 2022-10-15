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

#include <sys/ioctl.h>
#include <sys/queue.h>

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

#define  die(...)  err(EXIT_FAILURE, __VA_ARGS__)
#define diex(...) errx(EXIT_FAILURE, __VA_ARGS__)

struct line_item {
	char *buffer;
	STAILQ_ENTRY(line_item) list;
};

STAILQ_HEAD(lines_head, line_item);

static void center(FILE *);
static void center_by_longest(FILE *);
static void println(const char *, size_t);
static long polong(char *, const char *);
static int cols(void);
static int utf8len(const char *);
static int noesclen(const char *);
static int matchesc(const char *);
static int cnttabs(const char *);

extern int optind;
extern char *optarg;

int rval;
long width = -1;
long tabwidth = 8;
bool rflag;
int (*lenfunc)(const char *) = noesclen;

/* Return the length of the line `s' which contains `tabs' amount of tab
 * characters.
 */
static inline int
calclen(const char *s, int tabs)
{
	return lenfunc(s) - tabs + tabs * tabwidth;
}

int
main(int argc, char **argv)
{
	int opt;
	void (*centerfunc)(FILE *) = center;

	while ((opt = getopt(argc, argv, ":elrt:w:")) != -1) {
		switch (opt) {
		case 'e':
			lenfunc = utf8len;
			break;
		case 'l':
			centerfunc = center_by_longest;
			break;
		case 'r':
			rflag = true;
			break;
		case 't':
			tabwidth = polong(optarg, "tab width");
			break;
		case 'w':
			width = polong(optarg, "output width");
			break;
		default:
			fprintf(
				stderr,
				"Usage: %s [-elr] [-t width] [-w width] [file ...]\n",
				argv[0]
			);
			exit(EXIT_FAILURE);
		}
	}

	if (width == -1)
		width = cols();

	argc -= optind;
	argv += optind;

	if (argc == 0)
		centerfunc(stdin);
	else do {
		if (strcmp(*argv, "-") == 0)
			centerfunc(stdin);
		else {
			FILE *fp;
			if ((fp = fopen(*argv, "r")) == NULL) {
				warn("fopen");
				rval = EXIT_FAILURE;
			} else {
				centerfunc(fp);
				fclose(fp);
			}
		}
	} while (*++argv);

	return rval;
}

/* Write the contents of the file pointed to by `fp' center aligned to the
 * standard output.
 */
void
center(FILE *fp)
{
	static char *line = NULL;
	static size_t bs = 0;

	while (getline(&line, &bs, fp) != -1) {
		int len, tabs;
		tabs = cnttabs(line);
		len = calclen(line, tabs);

		println(line, len);
	}
	if (ferror(fp)) {
		warn("getline");
		rval = EXIT_FAILURE;
	}
}

/* Same as center(), but aligns all lines according to the longest line. Great
 * for centering code.
 */
void
center_by_longest(FILE *fp)
{
	int tabs;
	size_t curr_len, longest = 0;
	struct lines_head list_head;
	struct line_item *line, *tmp;
	static char *buffer = NULL;
	static size_t bs = 0;

	STAILQ_INIT(&list_head);

	/* Collect all input lines in a list and find the longest */
	while (getline(&buffer, &bs, fp) != -1) {
		line = malloc(sizeof(struct line_item));
		if (!line) {
			warn("malloc");
			rval = EXIT_FAILURE;
			return;
		}

		line->buffer = buffer;
		STAILQ_INSERT_TAIL(&list_head, line, list);
		tabs = cnttabs(buffer);
		curr_len = calclen(buffer, tabs);
		if (curr_len > longest)
			longest = curr_len;

		/* Reset buffer and bs so getline() can alloc a new buffer */
		buffer = NULL;
		bs = 0;
	}

	if (ferror(fp)) {
		warn("getline");
		rval = EXIT_FAILURE;
		return;
	}

	/* Output lines aligned to the longest and free them */
	line = STAILQ_FIRST(&list_head);
	while (line != NULL) {
		println(line->buffer, longest);
		tmp = STAILQ_NEXT(line, list);
		free(line->buffer);
		free(line);
		line = tmp;
	}
}

/* Return the column width of the output device if it is a TTY.  If it is not a
 * TTY then return -1.
 */
int
cols(void)
{
	char *s;
	struct winsize w;

	if ((s = getenv("COLUMNS")) != NULL)
		return polong(s, "output width");

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		if (errno == ENOTTY)
			return 80;
		else
			die("ioctl");
	}

	return w.ws_col;
}

/* Return the length of the UTF-8 encoded string `s' in characters, not bytes */
int
utf8len(const char *s)
{
	int l = 0;

	for (; *s; s++)
		l += *s == '\b' ? -1 : (*s & 0xC0) != 0x80;

	if (l > 0 && s[-1] == '\n')
		l--;

	return l;
}

/* Return the length of the UTF-8 encoded string `s' in characters, not bytes.
 * Additionally this function ignores ANSI color escape sequences when computing
 * the length.
 */
int
noesclen(const char *s)
{
	int off = 0;
	const char *d = s;

	while ((d = strchr(d, ESC)) != NULL)
		off += matchesc(++d);

	return utf8len(s) - off;
}

/* Return the length of the ANSI color escape sequence at the beginning of the
 * string `s'.  If no escape sequence is matched then 0 is returned.  The local
 * variable `c' is initialized to 3 in order to account for the leading ESC, the
 * following `[', and the trailing `m'.
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

/* Return the amount of tab characters present in the string `s'. */
int
cnttabs(const char *s)
{
	int cnt = 0;
	const char *p = s - 1;
	while ((p = strchr(p + 1, '\t')) != NULL)
		cnt++;
	return cnt;
}

/* Print the line `s' with a length of `len' centered to standard output. */
void
println(const char *s, size_t len)
{
	for (int i = (width - len) / 2; i >= 0; i--)
		putchar(' ');

	if (rflag) {
		for (int i = 0; s[i]; i++) {
			if (s[i] == '\t') {
				for (int j = 0; j < tabwidth; j++)
					putchar(' ');
			} else
				putchar(s[i]);
		}
	} else
		fputs(s, stdout);
}

/* Parse `optarg' as a long and store the result in `n'.  If `optarg' is less
 * than 0 then print a diagnostic message with the variable name `s' and exit.
 */
long
polong(char *s, const char *e)
{
	long n;
	char *endptr;
	n = strtol(s, &endptr, 0);
	if (*s == '\0' || *endptr != '\0')
		diex("invalid integer '%s'", s);
	if (n < 0)
		diex("%s must be >= 0", e);
	if (errno == ERANGE || n > INT_MAX) \
		warnx("potential overflow of %s", e);
	return n;
}
