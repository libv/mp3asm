/*  mp3asm: an mp3 frameeditor.
 *
 *  stream.c: readin, writing & sorting of streams
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
#include "utils.h"
#include "stream.h"
#include "frame.h"

/* utils.c */

extern void *tmalloc (size_t size);
extern void print_all (int verb);
extern void print_log (int verb);
extern FILE *mp3wopen(char **name, int layer);
extern buffer_t *init_buf (int size);
extern void free_buf(buffer_t *buffer);
extern int fill_buf_from_file (buffer_t *buffer, FILE *file);
extern int rem_buf (buffer_t *buffer, int count);
extern int cut_buf (buffer_t *buffera, buffer_t *bufferb, int count);
extern int print_buf (buffer_t *buffer);

/* tag.c */

extern int get_tag_v1 (stream_t *stream, buffer_t *buffer);
extern int write_tag_v1 (stream_t *stream, buffer_t *buffer);

/* frame.c */

extern void free_first_frame (stream_t *stream);
extern int read_frame(stream_t *stream, buffer_t *filebuf, buffer_t *databuf);
extern int remove_dead_frames (stream_t *stream);
extern int remove_prev_frames (stream_t *stream, frame_t *frame, long count);
extern int remove_next_frames (stream_t *stream, frame_t *frame, long count);
extern int calc_backref (stream_t *stream);
extern int write_frames (stream_t *stream, FILE *file); 

#define FILE_BUF_SIZE 20 /* kilobytes */
#define DATA_BUF_SIZE 4 /* kilobytes */

/*
 * init_stream: inits and returns a fresh stream.
 *
 */

static stream_t *
init_stream(void) 
{
  stream_t *stream;

  stream = tmalloc(sizeof(stream_t));
  stream->maj_version = 0;
  stream->min_version = 0;
  stream->layer = 0;
  stream->samples = 0;
  stream->isize = 0;
  stream->freq = 0;
  stream->cbr = 1;
  stream->avkbps = 0;
  stream->mode = 0;
  stream->crc = 0;
  stream->private = 0;
  stream->copyrighted = 0;
  stream->original = 0;
  stream->count = -1;
  stream->tag = NULL;

  stream->first = NULL;
  stream->last = NULL;

  return(stream);
} 

/*
 *  Stream init & free.
 *
 */

void
free_stream(stream_t *stream)
{
  while (stream->first)
    free_first_frame (stream);
  free(stream);
} 

/*
 * read_stream: reads and returns a fresh stream.
 *
 */

stream_t *
read_stream (FILE *file)
{
  int eof = 0, eob;
  buffer_t *filebuf = init_buf (FILE_BUF_SIZE * 1024), *databuf = init_buf (DATA_BUF_SIZE * 1024);
  stream_t *stream = init_stream ();

  while (!eof)
    {      
      switch (fill_buf_from_file (filebuf, file)) /* fill up the buf */
	{
	case -1:
	  perror("read_stream ()");
	  free_buf (filebuf);
	  free_buf (databuf);
	  free_stream (stream);
	  return (NULL);
	case 0:
	  eof++;
	}
      eob = 0;
      
      while (!eob)
	switch (read_frame (stream, filebuf, databuf))
	  {
	  case -1:
	    perror("read_stream ()");
	    free_buf (filebuf);
	    free_buf (databuf);
	    free_stream (stream);
	    return (NULL);
	  case 0:
	    eob++;
	  }
    }
  if (filebuf->used)
    {
      if ((filebuf->used + databuf->used) > databuf->size)
	{
	  if (filebuf->used > databuf->size)
	    rem_buf (filebuf, filebuf->used - databuf->size);
	  rem_buf (databuf, (databuf->used + filebuf->used) - databuf->size);
	}
      cut_buf (filebuf, databuf, filebuf->used);
    }
  free_buf (filebuf);

  fprintf (stderr, "databuf used: %d\n", databuf->used);

  if (get_tag_v1 (stream, databuf))
    fprintf (stderr, "the tag is: %s\n", stream->tag);

  rem_buf (databuf, databuf->used);
  free_buf (databuf);

  return (stream);
}

/*
 * print_stream_inf: should print to logfile (whichever that is)
 *             
 */

void
print_stream_inf(stream_t *stream, char *name)
{
  sprintf (log.buf, "%s: Info on file %s:\n", me, name);
  print_all (1);
  sprintf (log.buf, "        MPEG %d layer %d", stream->maj_version, stream->layer);
  print_all (1);

  if (!stream->cbr)
    sprintf (log.buf, " VBR [avg. %.0f kbps], ", stream->avkbps);
  else 
    sprintf (log.buf, " at %.0f kbps, ", stream->avkbps);

  print_all (1);
  sprintf (log.buf, "sampled at %d.\n", stream->freq);
  print_all (1);
  
  sprintf(log.buf, "        ");
  print_all (1);

  switch (stream->mode)
    {
    case 0:
      sprintf(log.buf, "Stereo, ");
      break;
    case 1:
      sprintf(log.buf, "Joint Stereo, ");
      break;
    case 2:
      sprintf(log.buf, "Dual Channel, ");
      break;
    case 3:
      sprintf(log.buf, "Mono, ");
      break;
    }
  print_all (1);
  
  switch (stream->crc)
    {
    case 0:
      sprintf(log.buf, "not protected by CRC.\n");
      break;
    case 1:
      sprintf(log.buf, "protected by CRC.\n");
      break;
    }
  print_all (1);

  sprintf(log.buf, "        ");
  print_all (1);

  if (stream->private) 
    sprintf(log.buf, "private, ");
  else
    sprintf(log.buf, "not private, ");
  print_all (1);
  
  if (stream->copyrighted) 
    sprintf(log.buf, "copyrighted, ");
  else
    sprintf(log.buf, "not copyrighted, ");
  print_all (1);

  if (stream->original) 
    sprintf(log.buf, "original.\n");
  else
    sprintf(log.buf, "not original.\n");
  print_all (1);

  sprintf(log.buf, "        %ld frames at %d samples per frame, infosize = %d.\n", stream->count, stream->samples, stream->isize);
  print_all (1);
}

/*
 * process_stream: 
 *
 */
int
process_stream (stream_t *stream, long skipframes, long lastframe)
{
  /*  this function is optimised for skipframes || lastframe to equal 0
   *  As this will very often be the case.
   *  A gtk frontend will one day make it much easier to use & program
   */

  long count = 0;
  frame_t *frame = stream->first;

  if (skipframes)
    {
      if (skipframes >= stream->count)
	{
	  sprintf (log.buf, "Cannot skip %ld frames... No frames left to write.\n", skipframes);
	  print_all (0);
	  exit(EX_USAGE);
	}
      
      while (frame)
	{
	  if (count == skipframes)
	    {
	      remove_prev_frames (stream, frame, count);
	      break;
	    }
	  frame = frame->next;
	  count++;
	}
    }  

  if (lastframe)
    {
      if (lastframe > stream->count)
	{
	  sprintf (log.buf, "File has less frames (%ld) than specified (%ld)... Reading all the way to the end.\n", stream->count, lastframe);
	  print_all (0);
	}

      while (frame)
	{
	  if (count == lastframe)
	    {
	      remove_next_frames (stream, frame, count);
	      break;
	    }
	  frame = frame->next;
	  count++;
	}
    }  

  remove_dead_frames (stream);
  
  return (1); /* succes */
}

/*
 * write_stream: 
 *
 */
int
write_stream (stream_t *stream, char **filename)
{
  FILE *file = mp3wopen (filename, stream->layer);
  
  calc_backref (stream);
  
  write_frames (stream, file);

  return (1); /* succes */
}

/* EOF */
