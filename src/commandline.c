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
#include <string.h>
#include "logging.h" // for LOG_MIN/MAX
#include "tools.h"
#include "commandline.h"

/**
 ** 
 ** Range arithmetic
 **
 **/

/*
 * Remove range.
 */
static void
commandline_range_remove (struct commandline_file_t *file, int position)
{
  int i;
  
  nfree(file->ranges[position]);
  
  for (i = position + 1; i < file->range_count; i++)
    file->ranges[i - 1] = file->ranges[i];
  
  file->range_count--;
  
  if (file->range_count)
    file->ranges = nrealloc (file->ranges, file->range_count * sizeof (struct range_t *));
  else {
    nfree (file->ranges);
    file->ranges = NULL;
  }
}

/*
 * Insert range.
 */
static void
commandline_range_insert (struct commandline_file_t *file, int position, int start, int stop)
{
  int i;
  file->range_count++;
  file->ranges = nrealloc (file->ranges, file->range_count * sizeof (struct range_t *));

  for (i = file->range_count - 1; i > position; i--)
    file->ranges[i] = file->ranges[i - 1];

  file->ranges[position] = nmalloc (sizeof (struct range_t));
  file->ranges[position]->start = start;
  file->ranges[position]->stop = stop;
}

/*
 *
 */
static void
commandline_range_include (struct commandline_file_t *file, int start, int stop)
{
  int i = 0;
    
  if (file->range_count == 0) {
    commandline_range_insert (file, 0, start, stop);
    return;
  }

  // find the position of start in the range and add accordingly.
  if (start == -1)
    commandline_range_insert (file, 0, start, stop);
  else {
    while (i < file->range_count) {
      if (start < file->ranges[i]->start) {
	commandline_range_insert (file, i, start, stop);
	break;
      } else if (file->ranges[i]->stop == -1)
	return;
      else if (start <= (file->ranges[i]->stop + 1)) {
	// + 1 so that adjacent ranges are taken together.
	commandline_range_insert (file, i, file->ranges[i]->start, stop);
	break;
      }
      i++;
    }
    if (i == file->range_count) {
      commandline_range_insert (file, file->range_count, start, stop);
      return;
    }
  }

  // what about stop
  // i is always >= 0 now. so (++i - 1) is always valid
  i++;
  if (stop == -1) {
    while (i < file->range_count)
      commandline_range_remove (file, i);
  } else {
    while (i < file->range_count) {
      if (stop < (file->ranges[i]->start - 1)) // - 1: adjacent ranges.
	return;
      if ((file->ranges[i]->stop == -1) || (stop < file->ranges[i]->stop))
	file->ranges[i - 1]->stop = file->ranges[i]->stop;
      commandline_range_remove (file, i);
    }
  }
}

/*
 *
 */
static void
commandline_range_exclude (struct commandline_file_t *file, int start, int stop)
{
  int i = 0;

  if (file->range_count <= 0) {
    // nothing set yet but we are already excluding: exclude from full range please.
    if (stop != -1)
      commandline_range_insert (file, 0, stop + 1, -1);
    if (start)
      commandline_range_insert (file, 0, 0, start - 1);
    return; 
  }

  if (start != -1) {
    while (i < file->range_count) {
      if (start <= file->ranges[i]->start)
	break;
      else if ((file->ranges[i]->stop == -1) || (start <= file->ranges[i]->stop)) {
	commandline_range_insert (file, i, file->ranges[i]->start, start - 1);
	i++;
	break;
      }
      i++;
    }
    if (i == file->range_count) // this range is outside of the existing ranges, return
      return;
  }

  // remove everything until stop
  if (stop == -1) {
    while (i < file->range_count)
      commandline_range_remove (file, i);
  } else {
    while (i < file->range_count) {
      if (stop < file->ranges[i]->start)
	return;
      else if ((file->ranges[i]->stop == -1) || (stop < file->ranges[i]->stop)) {
	file->ranges[i]->start = stop + 1;
	return;
      } 
      commandline_range_remove (file, i);
    }
  }
}

/**
 **
 ** Reducing code replication, slightly.
 **
 **/

/*
 * Add to a commandline_file_t array and return the added pointer
 */
static struct commandline_file_t *
commandline_file_new (struct commandline_file_t ***files, int *count)
{
  struct commandline_file_t *file = nmalloc (sizeof (struct commandline_file_t));
  file->filename = NULL;
  file->ranges = NULL;
  file->range_count = 0;
  file->tag = 0;

  (*count)++;
  *files = nrealloc (*files, *count * sizeof (struct commandline_file_t *));
  (*files)[*count - 1] = file;
  return file;
}

/*
 * Get the last commandline_file_t in the array, if none, create.
 */
static struct commandline_file_t *
commandline_file_last (struct commandline_file_t ***files, int *count)
{
  if (*files)
    return (*files)[*count - 1];
  else
    return (commandline_file_new (files, count));
}

/**
 **
 ** Some pretty print.
 **
 **/

/*
 *
 */
static void
commandline_print_help (void)
{
  fprintf (stderr, "mp3asm [-(global_flags)] inputfile(s) [-o outputfile(s)]\n\n");
  fprintf (stderr, "global_flags:\n");
  fprintf (stderr, "    q: decrease verbosity\n");
  fprintf (stderr, "    v: increase verbosity\n");
  // add more flags please!!!
  fprintf (stderr, "\n");
  fprintf (stderr, "file: filename [-flags] [range]\n");
  fprintf (stderr, "    file_flags:\n");
  fprintf (stderr, "        t: (input ) use the tag of this file (use only once in list)\n");
  fprintf (stderr, "           (output) used to tag (tags all by default if inputtag exists)\n");
  fprintf (stderr, "           negate this flag by placing \"%c\" in front of it\n", COMMANDLINE_EXCLUDE);
  fprintf (stderr, "    range:\n");
  fprintf (stderr, "        can be sole number(s) and ranges combined, all can be reversed with \"%c\"\n", COMMANDLINE_EXCLUDE);
  fprintf (stderr, "    ranges: <number>:<number> (boundaries are included)\n");
  fprintf (stderr, "        examples: these ranges are the exact opposite of eachother:\n");
  fprintf (stderr, "              5 32:50\n");
  fprintf (stderr, "              :31 %c5 51: or :4 6: %c32:50\n", COMMANDLINE_EXCLUDE, COMMANDLINE_EXCLUDE);
}

/*
 *
 */
static void
commandline_print_version (void)
{
  fprintf (stderr, "Mp3Asm version %s\n", VERSION);
  fprintf (stderr, "(c) 1996-1997  Olli Fromme\n");
  fprintf (stderr, "(c) 2001-2003  Luc Verhaegen\n\n");
}

/**
 **
 ** Some real work.
 **
 **/

/*
 *
 */
static void
commandline_init (void)
{
  commandline.verbosity = DEFAULT_VERBOSITY;

  commandline.log_file = NULL;
  commandline.read_buffer_size = DEFAULT_READ_BUFFER_SIZE;
  commandline.write_buffer_size = DEFAULT_WRITE_BUFFER_SIZE;

  commandline.input_count = 0;
  commandline.inputs = NULL;
  
  commandline.output_count = 0;
  commandline.outputs = NULL;
}

/*
 * returns:
 * -1: error/exit: message sent, please exit program
 *  0: goto next stage:
 *  1: advance count once
 *  2: advance count twice
 * ...
 * 0: if -o found and input should be skipped then -o will be fed to input parser which
 *    will return 0 itself.
 */
static int
commandline_parse_global (int argc, char *argv[], int count)
{
  char *p = argv[count];

  if (*p != '-') // this cant be a global flag then.
    return 0;

  p++;
  
  if (*p == '-') {
    if (!strcmp ("version", p))
      commandline_print_version ();
    else if (!strcmp ("help", p))
      commandline_print_help ();
    else
      fprintf (stderr, "Error: Unknown flag '%c' in '%s' [arg. %d]\n", *p, argv[count], count);
    return -1;
  }

  while (*p) {
    switch (*p) {
    case 'v':
      if (commandline.verbosity < LOG_MAX)
	commandline.verbosity++;
      break;
    case 'q':
      if (commandline.verbosity > LOG_MIN)
	commandline.verbosity--;
      break;
    case 'h':
      commandline_print_help ();
      return -1;
    case 'o': // go to input, -o will get it to jump to output
      return 0;
    default:
      fprintf (stderr, "Error: Unknown flag '%c' in '%s' [arg. %d]\n", *p, argv[count], count);
      return -1;
    }
    p++;
  }
  return 1;
}

/*
 *
 */
static int
commandline_parse_file_input (int argc, char *argv[], int count)
{
  char *p = argv[count];
  struct commandline_file_t *file;
  
  if (*p == '-') { // a flag.
    int exclude = 0;
    p++;
    
    while (*p) {
      switch (*p) {
      case 'o': // output
	// if no inputs yet; handle at commandline_test
	return 0;
      case COMMANDLINE_EXCLUDE:
	exclude = !exclude;
	break;
      case 't':
	file = commandline_file_last (&commandline.inputs, &commandline.input_count);
	if (exclude)
	  file->tag = -1;
	else
	  file->tag = 1;
	break;
      default:
	fprintf (stderr, "Error: Unknown flag '%c' in '%s' [arg. %d]\n", *p, argv[count], count);
	return -1;
      }
      p++;
    }
  } else { // filename or range
    int exclude = 0, start = 0, delim = 0, value = -1;
    if (*p == COMMANDLINE_EXCLUDE) {
      exclude = !exclude;
      p++;
    }
    while (*p) {
      if ((*p >= '0') && (*p <= '9')) {
	if (value < 0)
	  value = *p - '0';
	else {
	  value *= 10;
	  value += *p - '0';
	}
      } else if (*p == ':') {
	delim++;
	if (value >= 0) {
	  start = value;
	  value = -1;
	}
      } else { // must be a filename then
	file = commandline_file_new (&commandline.inputs, &commandline.input_count);
	file->filename = strcpy (nmalloc (strlen(argv[count]) + 1), argv[count]);
	return 1;
      }
      p++;
    }

    // this is a range.

    if (!delim) // lone frame
      start = value;
    
    file = commandline_file_last (&commandline.inputs, &commandline.input_count);

    if (delim && (value >= 0) && (value < start)) {
      if (exclude)
	commandline_range_exclude (file, value, start);
      else
	commandline_range_include (file, value, start);
    } else {
      if (exclude)
	commandline_range_exclude (file, start, value);
      else
	commandline_range_include (file, start, value);
    }
  }
  return 1;
}

/*
 *
 */
static int
commandline_parse_file_output (int argc, char *argv[], int count)
{
  char *p = argv[count];
  struct commandline_file_t *file;
  
  if (*p == '-') { // a flag.
    int exclude = 0;
    p++;
    
    while (*p) {
      switch (*p) {
      case 'o': // first time output, just skip this flag.
	break;
      case COMMANDLINE_EXCLUDE:
	exclude = !exclude;
	break;
      case 't':
	file = commandline_file_last (&commandline.outputs, &commandline.output_count);
	if (exclude)
	  file->tag = -1;
	else
	  file->tag = 1;
	break;
      default:
	fprintf (stderr, "Error: Unknown flag '%c' in '%s' [arg. %d]\n", *p, argv[count], count);
	return -1;
      }
      p++;
    }
  } else { // filename or range
    int exclude = 0, start = 0, delim = 0, value = -1;
    if (*p == COMMANDLINE_EXCLUDE) {
      exclude = !exclude;
      p++;
    }
    while (*p) {
      if ((*p >= '0') && (*p <= '9')) {
	if (value < 0)
	  value = *p - '0';
	else {
	  value *= 10;
	  value += *p - '0';
	}
      } else if (*p == ':') {
	delim++;
	if (value >= 0) {
	  start = value;
	  value = -1;
	}
      } else { // must be a filename then
	file = commandline_file_new (&commandline.outputs, &commandline.output_count);
	file->filename = strcpy (nmalloc (strlen(argv[count]) + 1), argv[count]);
	return 1;
      }
      p++;
    }

    // this is a range.

    if (!delim) // lone frame
      start = value;
    
    file = commandline_file_last (&commandline.outputs, &commandline.output_count);
    
    if (delim && (value >= 0) && (value < start)) {
      if (exclude)
	commandline_range_exclude (file, value, start);
      else
	commandline_range_include (file, value, start);
    } else {
      if (exclude)
	commandline_range_exclude (file, start, value);
      else
	commandline_range_include (file, start, value);
    }
  }
  return 1;
}

/*
 *
 */
static int
commandline_parse (int argc, char *argv[])
{
  int count = 1, return_value;
  int (*parser) (int argc, char *argv[], int count) = commandline_parse_global;

  // handle global flags
  while (count < argc) {
    return_value = parser (argc, argv, count);
    
    if (!return_value) { // move to next parser
      if (parser == commandline_parse_global)
	parser = commandline_parse_file_input;
      else if (parser == commandline_parse_file_input)
	parser = commandline_parse_file_output;
      else // end of the ride
	return 0;
    }

    if (return_value < 0)
      return -1;
    
    count += return_value;
  }
  return 0;
}

/*
 *
 */
static int
commandline_test (void)
{
  // fill me
  
  if (!commandline.input_count)
    // use stdin
    commandline_file_new (&commandline.inputs, &commandline.input_count);
  
  return 0;
}

/*
 * Temporary stuff, till the parsing works out ok
 */
static void
commandline_print (void)
{
  int i, j;
  struct commandline_file_t *file;

  fprintf (stderr, "Parsed commandline:\n");
  fprintf (stderr, "- global:\n");
  fprintf (stderr, "   - verbosity: %d\n", commandline.verbosity);

  fprintf (stderr, "- inputs: %d\n", commandline.input_count);
  for (i=0; i < commandline.input_count; i++) {
    file = commandline.inputs[i];
    fprintf (stderr, "   - input %d:\n", i);
    if (file->filename)
      fprintf (stderr, "      - filename: %s\n", file->filename);
    else
      fprintf (stderr, "      - filename: [stdin]\n");
    fprintf (stderr, "      - use tag: ");
    switch (file->tag) {
    case -1:
      fprintf (stderr, "explicit no\n");
      break;
    case 0:
      fprintf (stderr, "no\n");
      break;
    case 1:
      fprintf (stderr, "explicit yes\n");
      break;
    }
    fprintf (stderr, "      - range: ");
    if (file->range_count <= 0)
       fprintf (stderr, "full");
    else 
      for (j = 0 ; j < file->range_count; j++) {
	if (file->ranges[j]->stop != -1) 
	  fprintf (stderr, " %d:%d ", file->ranges[j]->start, file->ranges[j]->stop);
	else
	  fprintf (stderr, " %d:end ", file->ranges[j]->start);
      }
    fprintf (stderr, "\n");
  }

  fprintf (stderr, "- outputs: %d\n", commandline.output_count);
  for (i=0; i < commandline.output_count; i++) {
    file = commandline.outputs[i];
    fprintf (stderr, "   - output %d:\n", i);
    if (file->filename)
      fprintf (stderr, "      - filename: %s\n", file->filename);
    else
      fprintf (stderr, "      - filename: [stdout]\n");
    fprintf (stderr, "      - use tag: ");
    switch (file->tag) {
    case -1:
      fprintf (stderr, "explicit no\n");
      break;
    case 0:
      fprintf (stderr, "yes\n");
      break;
    case 1:
      fprintf (stderr, "explicit yes\n");
      break;
    }
    fprintf (stderr, "      - range: ");
    if (file->range_count <= 0)
       fprintf (stderr, "full");
    else 
      for (j = 0 ; j < file->range_count; j++)
	fprintf (stderr, " %d:%d ", file->ranges[j]->start, file->ranges[j]->stop);
    fprintf (stderr, "\n");
  }
};

/*
 *
 */
int
handle_commandline (int argc, char *argv[])
{
  commandline_init ();

  if (commandline_parse (argc,argv))
    return -1;
  
  if (!commandline_test ()) {
    commandline_print ();
    return 0;
  }

  return -1;
}

/* EOF */
