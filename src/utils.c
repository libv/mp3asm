/*  mp3asm: an mp3 frameeditor.
 *
 *  utils.c : all sorts of handy little functions.
 *
 *  Copyright (C) 2001  Luc Verhaegen (_Death_@Undernet(IRC))
 *                                    <dw_death_@hotmail.com>
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
#include <stdlib.h>
#include <argz.h>
#include <stdio.h>
#include "mp3asm.h"

/* fe.c */
void fe_simple_dialog (char *message);

/*
 * tmalloc: mallocs cleanly
 *
 */
void
*tmalloc (size_t size)
{
  void *mem;
  
  if (!(mem = malloc(size)))
    {
      fprintf(stderr, "Out of memory.");
      exit(ENOMEM);
    }
  return (mem);
}

/*
 * tmalloc: mallocs cleanly
 *
 */
void
*tcalloc (int n, size_t size)
{
  void *mem;
  
  if (!(mem = calloc(n, size)))
    {
      fprintf(stderr, "Out of memory.");
      exit(ENOMEM);
    }
  return (mem);
}

/*
 * trealloc: reallocs cleanly.
 *
 */
void *
trealloc (void *ptr, size_t size)
{
  void *mem;
  if (!(mem = realloc(ptr, size)))
    {
      fprintf(stderr, "Out of memory.");
      exit(ENOMEM);
    }
  return (mem);
}

unsigned char *
mp3_read (int filenr, long pos, size_t size)
{
  FILE *tfile = file[filenr]->file;
  long curpos = ftell(tfile);
  unsigned char *buf = tmalloc (size);

  if (curpos < 0)
    {
      char buf1[1024];
      sprintf (buf1, "Unable to properly read from\n %s.\n", file[filenr]->name);
      fe_simple_dialog (buf1);
      return (NULL);
    }
  if (curpos > pos)
    fseek (tfile, pos - curpos, SEEK_CUR);
  else if (curpos < pos)
    fseek (tfile, pos, SEEK_SET);

  fread (buf, 1, size, tfile);
  return (buf);
}

/* EOF */
