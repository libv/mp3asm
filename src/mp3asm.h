/*  mp3asm: an mp3 frameeditor.
 *
 *  mp3asm.h : main includes file.
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

#ifndef HAVE_MP3ASM_H
#define HAVE_MP3ASM_H

#include <stdio.h>
/*
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <argz.h>
#include <ctype.h>*/

typedef struct mp3_t
{
  struct mp3_gui_t *gui;
  struct stream_t *stream;
} mp3_t;

extern int mp3s;
extern mp3_t **mp3;

typedef struct file_t
{
  char *name;
  unsigned char *head;
  long size;
  FILE *file;
  
} file_t; 

extern int files;
extern file_t **file;

#endif /* HAVE_MP3ASM_H */

/* EOF */

