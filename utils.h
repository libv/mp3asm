/*
 *   utils.h
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Fri Apr 18 15:27:45 MET DST 1997
 */

#ifndef HAVE_UTILS_H
#define HAVE_UTILS_H

#include <stdlib.h>

#ifndef TRUE
#define FALSE 0
#define TRUE 1
typedef int bool;
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern char *me;
/*
 *   The name of the executable (without directory).
 *   Initialized by init().
 */

void out_of_memory (void);
/*
 *   Prints the program name followed by a message that there
 *   is not enough memory (to stderr) and exits with code 1.
 */

void *tmalloc (size_t size);
/*
 *   Wrapper for malloc() which checks the result and calls
 *   out_of_memory() if the requested memory could not be
 *   allocated.
 */

char *strndup (const char *src, int num);
/*
 *   Like strdup(), but limits the string length to at most
 *   <num> characters (not counting the terminating zero).
 *   The resulting string is always zero-terminated.
 *   Always allocates <num>+1 bytes, even if less space would
 *   be sufficient to store <src>.
 */

char *justify (char *str);
/*
 *   Removes leading and trailing whitespace, and compresses
 *   contained whitespace to single spaces.  Returns <str>.
 *   Whitespace is a sequence of ' ', '\t', '\r' and/or '\n'.
 */

void die (const char *func, const char *file);
/*
 *   Prints the executable name, <func> (if != NULL), <file>
 *   (if != NULL) and the error string corresponding to errno
 *   to stdout, followed by a newline, and exits the program
 *   with code 1 (failure).
 */

void utils_init (const char *argv0);
/*
 *   Initializes <me>.  Should be the first instruction in
 *   main(), with argv[0] as parameter.
 */

#endif /* HAVE_UTILS_H */

/* EOF */
