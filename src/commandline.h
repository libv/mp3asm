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
#ifndef HAVE_COMMANDLINE_H
#define HAVE_COMMANDLINE_H

#include <stdlib.h>
//#include <stdio.h>

#define COMMANDLINE_EXCLUDE '_'

struct range_t
{
  int start;
  int stop;
};

struct commandline_file_t
{
  char *filename; // NULL for stdin/stdout
  unsigned int range_count;
  struct range_t **ranges; // default: whole range
  int tag; 
  /* tagging:
   * input: arg specified: use this files tag, if exists, else no tag.
   *        not specified: use the tag ot first file
   *	    flag can only be used once.
   * output: // tag this file, default
   */
};

#define DEFAULT_VERBOSITY 2
#define DEFAULT_READ_BUFFER_SIZE 40
#define DEFAULT_WRITE_BUFFER_SIZE 40

struct commandline_t 
{
  int verbosity;
  //unsigned int max_file_size; // not used
  char *log_file;
  unsigned int read_buffer_size;
  unsigned int write_buffer_size; // wait until X frames are available before output
  //int variable; // not used
  // datasize too high: instead of writing an empty frame, try to solve the problem with vbr


  // move to a NULL terminated array and a (commandline_file_t *)++
  int input_count;
  struct commandline_file_t **inputs; // move to a NULL terminated array and a 

  int output_count;
  struct commandline_file_t **outputs;
};

struct commandline_t commandline;

#endif // HAVE_COMMANDLINE_H
