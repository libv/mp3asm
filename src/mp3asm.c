/*  mp3asm: an mp3 frameeditor.
 *
 *  mp3asm.c: something should hold int main :)
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
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sysexits.h>
#include "mp3asm.h"
#include "stream.h"

/* utils.c */
void *tmalloc (size_t size);
void *trealloc (void *ptr, size_t size);

/* fe.c */ 
extern void fe_init (int *argc, char **argv[]);
extern void fe_main (void);
extern void fe_update (void);

/* stream.c */
extern int open_stream (stream_t *stream, int filenr);

int mp3s;
mp3_t **mp3;
int files;
file_t **file;

/*
 * global_init:
 *
 */
static void
global_init (int argc, char *argv[])
{
  mp3 = NULL;
  mp3s = 0;
  file = NULL;
  files = 0;
}

/*
 * new_mp3: adds and loads a new mp3_t & stream
 *
 */
static int
new_mp3 (int filenr)
{
  stream_t *stream = NULL;
  int temp;

  fe_update ();

  if ((temp = open_stream (stream, filenr)) < 0)
    {
      if (temp == -1)
	return (-1);
      return (-2);
    }
  
  mp3s++;
  mp3 = trealloc (mp3, mp3s * sizeof (mp3_t *));
  mp3[mp3s - 1] = tmalloc (sizeof (mp3_t));
  mp3[mp3s - 1]->stream = stream;
  mp3[mp3s - 1]->gui = NULL;
   
  return (mp3s -1);
}

/*
 * new_file: adds and inits a new input file
 *
 */
static int
new_file (char *filename)
{
  struct flock flock;
  FILE *newfile;

  if ((newfile = fopen (filename, "r")) < 0)
    return (-1);
  
  /* lock file, so that nothing happens to it while mp3asm uses it */
  /* this way data can be specified in the struct by a long pointin to the offset */
  fe_update ();
  
  flock.l_type = F_RDLCK;
  flock.l_whence = SEEK_SET;
  flock.l_start = 0;
  flock.l_len = 0;

  if (fcntl(fileno(newfile), F_SETLK, &flock) < 0)
  return (-2);

  files++;
  file = trealloc(file, files * sizeof(file_t *));

  file[files - 1] = tmalloc (sizeof(file_t));
  file[files - 1]->name = strcpy(tmalloc(strlen(filename) + 1), filename);
  file[files - 1]->file = newfile;
  file[files - 1]->head = NULL;
  fseek (file[files - 1]->file, 0, SEEK_END);
  file[files - 1]->size = ftell (file[files - 1]->file);
  return (files - 1);
}

/*
 * open_input:
 *
 */
int
open_mp3 (char *filename)
{
  int filenr, temp; 
  
  if ((filenr = new_file(filename)) < 0)
    return (filenr);
  
  if ((temp = new_mp3(filenr)) < 0)
    {
      if (temp == -1)
	return (-3);
      return (-4);
    }
  
  return (0);
}

/*
 * main
 *
 */ 
int
main(int argc, char *argv[])
{ 
  global_init (argc, argv);

  fe_init(&argc, &argv);

  fe_main();

  exit(EX_OK);
}

/* EOF */
