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
#include <stdlib.h>
#include <sysexits.h>
#include "logging.h"
#include "input.h"
// commandline.c
int handle_commandline (int argc, char *argv[]);

// logging.c
void log_init (void);
// input.c
void input_init (void);
int input_open (void);
int input_read (void);
int input_frame_find_first (void);
int input_segment (int empty);

/*
 * main
 *
 */ 
int
main (int argc, char *argv[])
{ 
  int return_value;

  return_value = handle_commandline (argc, argv);
  if (return_value == -1)
    exit (EX_USAGE);
  else if (!return_value) {
    // here be dragons.
    log_init ();
    input_init ();

    if (input_open ())
      exit (EX_DATAERR);

    return_value = input_read ();
    if (return_value >= 0) {
      if (input_frame_find_first ())
	exit (EX_DATAERR);
      input_segment (0);
    }
    
  } 
  exit (EX_OK);
}

// EOF
