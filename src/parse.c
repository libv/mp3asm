/*  mp3asm: an mp3 frameeditor.
 *
 *  parse.c : parses the command line input.
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

#include "mp3asm.h"
#include "parse.h"
#include "utils.h"

/* mp3asm.c */
extern void new_input (void);

/* utils.c */
extern void *tmalloc (size_t size);
extern void print_std (int verb);

/*
 * Usage: shows the usage of the program.
 *
 */

static void
usage (void)
{ 
  sprintf (log.buf, "Usage: %s [options] file.mp3 [...]\n"
	   "   -h     shows the crap u have to make do"
	   " with cos the bastard who wrote this\n"
	   "          was too lame to make an info file.\n"
	   "   -v     increase verbosity level. (default = silent)\n"
	   "   -L     log file information to a file. (can be quite excessive)\n"
	   "   -i     display info from first frameheader and exit. (all files)\n"
	   "   -f     set the first frame to be read. (default = 0)\n"
	   "   -l     set the last frame to be read. (default = end of file)\n"
	   "   -r     set number of frames to read. ('l' & 'r' are conflicting flags)\n"
	   "   -t     Use id3-tag. (dont specify this more than once, ignored right now)\n"
	   "   -N     Not reporting illegal copyrighted material to the authorities.\n"
	   "   -o     file to output to. (default = none)\n", me);
  print_std (0);
  exit (EX_USAGE);
}

/*
 * Just to make the code a lil less unreadable :)
 *
 */

static void
bad_flag (char *string, int i)
{
  if (!i)
    sprintf (log.buf, "%s: Bad flag specified: \"%s\"\n", me, string);
  else 
    sprintf (log.buf, "%s: Bad flag specified: \"%s\" from \"%s\"\n", me, string+i, string);
  print_std (-1);
  exit (EX_USAGE);
}

static void
need_int (char *string, int i)
{
  if (!i)
    sprintf (log.buf, "%s: Bad flag specified, integer expected: \"%s\"\n", me, string);
  else 
    sprintf (log.buf, "%s: Bad flag specified, integer expected: \"%s\" from \"%s\"\n", me, string+i, string);

  print_std (-1);
  exit (EX_USAGE);
}

static void
double_flag (char *string)
{
  sprintf (log.buf, "%s: Flag specified twice: \"%s\"\n", me, string);
  print_std (-1);
  exit (EX_USAGE);
}

/*
 * Parses an individual argument.
 *
 */ 

static void 
parse_argument (int argc, char *argv[])
{
  /* dont mind whats in here too much, when all is converted the options will be checked */

  int strlength, i, j;

  for (i = 1; i < argc; ++i)
    {
      if (!(strncmp (argv[i], "-",1))) /* flag */
	{
	  strlength = strlen ((char *)argv[i]);
	  
	  for (j = 1 ; j < strlength; ++j)
	    {
	      switch (argv[i][j])
		{
		  
		case 'h': /* help */
		  usage ();
		  
		case 'v': /* verbosity */
		  ++verbosity;
		  sprintf (log.buf, "Verbosity = %d\n", verbosity);
		  print_std (5);
		  continue;

		case 'i': /* show header info and exit */
		  if((info)) 
		    double_flag (argv[i]);
		  ++info;
		  sprintf (log.buf, "Info = %d\n", info);
		  print_std (5);
		  continue;
		  
		case 'f': /* input - first frame to read */
		  if (input[inputs - 1]->skipframes) 
		    double_flag (argv[i]);
		  if ((++j) < strlength)
		    {            /* the int is defined right behind the flag */
		      if ((input[inputs - 1]->skipframes = atoi (argv[i]+j)))
			{
			  sprintf (log.buf, "%d skipframes = %d\n", inputs - 1, input[inputs - 1]->skipframes);
			  print_std (5);
			  continue;
			}
		      need_int (argv[i],j);
		    }
		  if (++i < argc)
		    {            /* the int is one whitespace away */
		      if ((input[inputs - 1]->skipframes = atoi(argv[i])))
			{
			  sprintf (log.buf, "%d skipframes = %d\n", inputs - 1, input[inputs - 1]->skipframes);
			  print_std (5);
			  continue;
			}
		      need_int(argv[i],0);
		    }
		  need_int (argv[i],0); /* sounds like there aint an int anywhere - lets exit! */
		  
		case 'l': /* last frame to be read */
		  if (input[inputs - 1]->lastframe) 
		    double_flag (argv[i]); 
		  if ((++j) < strlength)
		    {
		      if ((input[inputs - 1]->lastframe = atoi(argv[i]+j)))
			{
			  sprintf (log.buf, "%d lastframe = %d\n", inputs - 1, input[inputs - 1]->lastframe);
			  print_std (5);
			  continue;
			}
		      need_int(argv[i],j);
		    }
		  if (++i < argc)
		    {
		      if ((input[inputs - 1]->lastframe = atoi(argv[i])))
			{
			  sprintf (log.buf, "%d lastframe = %d\n", inputs - 1, input[inputs - 1]->lastframe);
			  print_std (5);
			  continue;
			}	
		      need_int (argv[i],0);
		    }
		  bad_flag (argv[i],0);		
		  
		case 'r': /* number of frames to read */
		  if (input[inputs - 1]->readframes) 
		    double_flag (argv[i]); 
       		  if ((++j) < strlength)
		    {
		      if ((input[inputs - 1]->readframes = atoi (argv[i]+j)))
			{
			  sprintf (log.buf, "%d readframes = %d\n",inputs - 1, input[inputs - 1]->readframes);
			  print_std (5);
			  continue;
			}      
		      need_int (argv[i],j);
		    }
		  if (++i < argc)
		    {
		      if ((input[inputs - 1]->readframes = atoi(argv[i])))
			{
			  sprintf (log.buf, "%d readframes = %d\n", inputs - 1, input[inputs - 1]->readframes);
			  print_std (5);
			  continue;
			}     
		      need_int (argv[i],0);
		    }
		  bad_flag (argv[i],0);
		  
		case 't': /* use the available id3 tag */
		  if (input[inputs - 1]->use_id3)
		    double_flag (argv[i]);

		  ++input[inputs -1]->use_id3;
		  sprintf(log.buf, "%d use_id3 = %d\n", inputs - 1, input[inputs - 1]->use_id3);
		  print_std (5);
		  continue;

		case 'o': /* output file  */
		  if (output->name)
		    {
		      sprintf (log.buf, "%s: Output file already specified.\n", me);
		      print_std (-1);
		      exit (EX_USAGE);
		    }
		  if ((++j) < strlength)
		    {
		      output->name = strcpy (tmalloc (strlen (argv[i]+j)), argv[i]+j);
		      sprintf (log.buf, "Output = %s\n", output->name);
		      print_std (5);
		      continue;
		    }
		  if (++i < argc)
		    { 
		      if (!(strncmp(argv[i], "-",1)))
			bad_flag (argv[i],0);
		      output->name = strcpy (tmalloc (strlen (argv[i])), argv[i]);
		      sprintf (log.buf, "Output = %s\n", output->name);
		      print_std (5);
		      continue;
		    }
		  bad_flag (argv[i],0);

		case 'L': /* log  */
		  if (log.name)
		    {
		      sprintf (log.buf, "%s: Log file already specified.\n", me);
		      print_std (-1);
		      exit (EX_USAGE);
		    }
		  if ((++j) < strlength)
		    {
		      log.name = strcpy (tmalloc (strlen (argv[i]+j)), argv[i]+j);
		      sprintf (log.buf, "Logfile = %s\n", log.name);
		      print_std (5);
		      continue;
		    }
		  if (++i < argc)
		    { 
		      if (!(strncmp(argv[i], "-",1)))
			bad_flag(argv[i],0);
		      log.name = strcpy (tmalloc (strlen(argv[i])), argv[i]);
		      sprintf (log.buf, "Logfile = %s\n", log.name);
		      print_std (5);
		      continue;
		    }
		
		default:
		  bad_flag (argv[i],0);
		}
	    }
	} else { /* filename here */
	  input[inputs - 1]->name = strcpy (tmalloc (strlen (argv[i])), argv[i]);
	  sprintf (log.buf, "%d input: %s\n", inputs - 1, input[inputs - 1]->name);
	  print_std (5);
	  new_input ();
	}
    }
}

/*
 * Check if the options r useable.
 *
 */

static void
check_options ()
{
  int id3=0, i;

  sprintf (log.buf, "inputs: %d\n", inputs);
  print_std (1);

  for (i = 0; i < inputs; ++i)
    {
      if (input[i]->readframes && input[i]->lastframe) {
	sprintf (log.buf, "%s: Invalid options given. Both l and r flags have been specified for \"%s\".\n", me, input[i]->name);
	print_std (-1);
	exit (EX_USAGE);
      }
      else if (input[i]->readframes)
	input[i]->lastframe = input[i]->skipframes + input[i]->readframes;
      else if (input[i]->lastframe) 
	input[i]->readframes = input[i]->lastframe - input[i]->skipframes;
      
      /*      if ((input[i]->skipframes > input[i]->lastframe) || (input[i]->skipframes && (input[i]->skipframes == input[i]->lastframe)))
      {
	sprintf (log.buf, "%s: Invalid options given. The number of frames to skip should be smaller than the number of the last frame to read for \"%s\".\n", me, input[i]->name);
	print_std (-1);
	exit (EX_USAGE);
	}*/
      
      if (input[i]->use_id3) {
	if (id3)
	  {
	    sprintf (log.buf, "%s: Invalid options given. More than one id3 tag specified for use.\n", me);
	    print_std (-1);
	    exit (EX_USAGE);
	  }
	else
	  ++id3;
      }
    }
  if (input[inputs - 1]->name == NULL)
    {
      if (input[inputs -1]->skipframes ||input[inputs -1]->lastframe || input[inputs -1]->readframes || input[inputs -1]->use_id3)
	{
	  sprintf (log.buf, "%s: Flags specified for input file, but no input file has been given.\n", me);
	  print_std (-1);
	  exit(EX_USAGE);
	}
      free (input[inputs - 1]);
      inputs--;
    }
  if (!inputs)
    {
      sprintf (log.buf, "%s: No input files specified.\n", me);
      print_std (-1);
      exit (EX_USAGE);
    }
  if (!info)
    {
      sprintf (log.buf, "%s: Program from Olli Fromme & _Death_.\n", me);
      print_std (-1);
      
      if (!output->name)
	sprintf (log.buf, "%s: Checking the following mp3 files for header errors:\n", me);
      else
	sprintf(log.buf, "%s: Taking input from:\n", me);
      print_std (1);
      
      for (i = 0; i < inputs; ++i) 
	{
	  sprintf (log.buf, "%s: input %d: %s from frame %d to %d [%d].\n", me, i, input[i]->name, input[i]->skipframes, input[i]->lastframe, input[i]->readframes);
	  print_std (2);
	}
      
      if (!output->name)
	sprintf (log.buf, "%s: No output file specified.\n", me);
      else
	sprintf (log.buf, "%s: Outputting to %s.\n", me, output->name);
      print_std (1);
    }
}

/*
 * Parses the arguments.
 *
 */

void
parse_args (int argc, char *argv[])
{
  if (argc < 2) 
    usage();
  
  parse_argument(argc, argv) ;
  check_options();
}

/* EOF */
