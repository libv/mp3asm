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
#ifndef HAVE_MP3ASM_H
#define HAVE_MP3ASM_H

#include <stdlib.h>
#include <stdio.h>
#include <obstack.h>
#define EXCLUDE '_'

struct range_t
{
  int start;
  int stop;
};

struct global_conf_t 
{
  char verbosity;
  int max_file_size;
  char *logfilename;
  int frame_count;
  int wait; // wait until X frames are available for restructuring before writing
  int variable; // instead of writing an empty frame, try to solve the problem with vbr
};

//#define FRAME_COUNT 40

struct input_file_t
{
  // argument list.

  char *filename; // NULL for stdin
  int range_count;
  struct range_t **ranges;
  int tag; // use the tag of this file - if none specified, use the tag of the first only
  // this can be specified only once on input



  FILE *file;
  int eof;

  int maj_version;  // 1, 2
  int min_version;  // 5 -> 2.5
  int layer; // 1, 2, 3
  int samples; // per frame
  int isize; // side info size
  int cbr; // boolean
  float avkbps;
  int freq;
  int mode; // 0 = stereo | 1 = joint stereo | 2 = dual chan | 3 = mono
  int crc;
  int private;
  int copyright;
  int original;

  unsigned char *head;
  int head_offset;
};

int input_count;
struct input_file_t **inputs;

struct output_file_t
{
  char *filename; // NULL for stdout
  int range_count;
  struct range_t **ranges; // writes the whole range per default
  int tag; // tag this file - this is true per default
};

struct global_conf_t global;

int output_count;
struct output_file_t **outputs;

#endif // HAVE_MP3ASM_H
