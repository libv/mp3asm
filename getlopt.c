/*
 *   getlopt.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Tue Apr  8 07:15:13 MET DST 1997
 */

#include <stdio.h>
#include <string.h>
#include "getlopt.h"

int loptind = 1;	/* index in argv[] */
int loptchr = 0;	/* index in argv[loptind] */
char *loptarg;		/* points to argument if present, else to option */

const topt
*findopt (int islong, char *opt, const topt *opts)
{
	if (!opts)
		return (0);
	while (opts->lname || opts->sname) {
		if (islong) {
			if (!strcmp(opts->lname, opt))
				return (opts);
		}
		else
			if (opts->sname == *opt)
				return (opts);
		opts++;
	}
	return (0);
}

int performoption (int argc, char *argv[], const topt *opt)
{
	int result = GLO_CONTINUE;

	if (!(opt->flags & 1)) /* doesn't take argument */
		if (opt->var)
			if (opt->flags & 2) /* var is *char */
				*((char *) opt->var) = (char) opt->value;
			else
				*((int *) opt->var) = opt->value;
		else
			result = opt->value ? opt->value : opt->sname;
	else { /* requires argument */
		if (loptind >= argc)
			return (GLO_NOARG);
		loptarg = argv[loptind++]+loptchr;
		loptchr = 0;
		if (opt->var)
			if (opt->flags & 2) /* var is *char */
				*((char **) opt->var) = strdup(loptarg);
			else
				*((int *) opt->var) = atoi(loptarg);
		else
			result = opt->value ? opt->value : opt->sname;
	}
	if (opt->func)
		opt->func(loptarg);
	return (result);
}

int getsingleopt (int argc, char *argv[], const topt *opts)
{
	char *thisopt;
	const topt *opt;
	static char shortopt[2] = {0, 0};

	if (loptind >= argc)
		return (GLO_END);
	thisopt = argv[loptind];
	if (!loptchr) { /* start new option string */
		if (thisopt[0] != '-' || !thisopt[1]) /* no more options */
			return (GLO_END);
		if (thisopt[1] == '-') /* "--" */
			if (thisopt[2]) { /* long option */
				loptarg = thisopt+2;
				loptind++;
				if (!(opt = findopt(1, thisopt+2, opts)))
					return (GLO_UNKNOWN);
				else
					return (performoption(argc, argv, opt));
			}
			else { /* "--" == end of options */
				loptind++;
				return (GLO_END);
			}
		else /* start short option(s) */
			loptchr = 1;
	}
	shortopt[0] = thisopt[loptchr];
	loptarg = shortopt;
	opt = findopt(0, thisopt+(loptchr++), opts);
	if (!thisopt[loptchr]) {
		loptind++;
		loptchr = 0;
	}
	if (!opt)
		return (GLO_UNKNOWN);
	else
		return (performoption(argc, argv, opt));
}

int getlopt (int argc, char *argv[], const topt *opts)
{
	int result;
	
	while ((result = getsingleopt(argc, argv, opts)) == GLO_CONTINUE);
	return (result);
}

void parselopts (int argc, char *argv[], const topt *opts, char *progname)
{
	int result;

	while ((result = getlopt(argc, argv, opts)))
		switch (result) {
			case GLO_UNKNOWN:
				fprintf (stderr, "%s: Unknown option \"%s\".\n",
					progname, loptarg);
				exit (1);
			case GLO_NOARG:
				fprintf (stderr, "%s: Missing argument for option \"%s\".\n",
					progname, loptarg);
				exit (1);
		}
}

/* EOF */
