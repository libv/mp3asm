/*  mp3asm: an mp3 frameeditor.
 *
 * 
 *
 *  Copyright (C) 2001-2003  Luc Verhaegen (_Death_@Undernet(IRC))
 *                                    <_death_@mp3asm.com>
 *  Copyright (C) 1996-1997 Olli Fromme <olli@fromme.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include "logging.h"
#include "commandline.h"

FILE *log_file;
int log_last_again; // keep track of \r

/*
 * !!!: implement log file != stderr
 */
int
log_init ()
{
  log_last_again = 0;

  if (!commandline.log_file)
    log_file = stderr;
  else
    fprintf (stderr, "MIND THE GAP: %s:%d\n", __FILE__, __LINE__);

  return 0;
}

/*
 * returns 1 if level provided is reached
 */
static int
log_level (int level) {
  return ((level <= commandline.verbosity));
}


/*
 * Implements logging to stderr or specified file.
 * !!! currently ignores file errors.
 */
void
log_once (int level, char *format,  ...)
{
  if (log_last_again) {
    fprintf (log_file, "\n");
    log_last_again = 0;
  }

  if (log_level(level)) {
    va_list va;
    va_start (va, format);
    vfprintf (log_file, format, va);
    fprintf (log_file, "\n");
  }
}

void
log_again (int level, char *format,  ...)
{
  log_last_again = 1;

  if (log_level(level)) {
    va_list va;
    va_start (va, format);
    vfprintf (log_file, format, va);
    fprintf (log_file, "\r");
  }
}

