/*
 *   utils.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Fri Apr 18 15:27:45 MET DST 1997
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

char *me;

void out_of_memory (void)
{
	fprintf (stderr, "%s: Out of memory.\n", me);
	exit (1);
}

void *tmalloc (size_t size)
{
	void *mem;

	if (!(mem = malloc(size)))
		out_of_memory();
	return (mem);
}

char *strndup (const char *src, int num)
{
	char *dst;

	if (!(dst = (char *) malloc(num+1)))
		return (NULL);
	dst[num] = '\0';
	return (strncpy(dst, src, num));
}

char *justify (char *str)
{
	int si = 0, di = 0, havespc = TRUE;
	char c;

	if (!str)
		return (str);
	/*LINTED*/
	while ((c = str[si])) {
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			if (havespc) {
				si++;
				continue;
			}
			else
				havespc = TRUE;
		else
			havespc = FALSE;
		str[di++] = str[si++];
	}
	if (di && havespc)
		di --;
	str[di] = '\0';
	return (str);
}

char *me;

void die (const char *func, const char *file)
{
	fprintf (stderr, "%s: ", me);
	if (func)
		fprintf (stderr, "%s(%s): ", func, file?file:"");
	else if (file)
		fprintf (stderr, "%s: ", file);
	fprintf (stderr, "%s\n", strerror(errno));
	exit (1);
}

void utils_init (const char *argv0)
{
	(me = strrchr(argv0, '/')) ? me++ : (me = (char *)argv0);
}

/* EOF */
