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
#include <stdio.h>
#include <stdlib.h>
#include <libio.h>
#include "stream.h"
#include "mp3asm.h"

/* utils.c */
void *tmalloc (size_t size);
void *trealloc (void *ptr, size_t size);
unsigned char *mp3_read (int filenr, long pos, size_t size);

/* block.c */
static void print_block (block_t *block);
block_t *init_block (int filenr, long pos, guint16 size, unsigned char type);
block_t *add_block (block_t **stream, int *count, int filenr,
		    unsigned long *pos, guint16 size, unsigned char type);
block_t *split_block (block_t **list, int *count, int *count2, int *pos);
/*
 * init_stream:
 *
 */
stream_t *
init_stream (void)
{
  stream_t *stream = tmalloc(sizeof(stream_t));
  
  memset (stream, 0, sizeof (stream_t));
  stream->list = NULL;
  
  return (stream);
}

/*
 * isheader: 
 *
 */
static int
is_header (unsigned char head[4])
{
  if ((head[0] == 0xff) && ((head[1] & 0xe0) != 0xe0)) /* bad syncbits */
    return (0);
  if ((head[1] & 0x18) == 0x08) /* bad mpeg version */
    return (0);
  if (!(head[1] & 0x06)) /* bad layer */
    return (0);
  if ((head[2] & 0xf0) == 0xf0) /* bad bitrate */
    return (0); 
  if ((head[2] & 0x0c) == 0x0c) /* bad sampling freq */
    return (0); 
  return(1);
}

/*
 * samestream: returns boolean for comparing 2 headers
 *             takes into account bitrate/mode def/padding
 */
static int
samestream (unsigned char head1[4], unsigned char head2[4])
{
  if ((head1[1] == head2[1]) && ((head1[2] & 0x0c) == (head2[2] & 0x0c)) && ((head1[3] & 0xc3) == (head2[3] & 0xc3)))
    return (1);
  return(0);
}

/*
 *   1st index:  0 = "MPEG 1.0",   1 = "MPEG 2.0"
 *   2nd index:  0 = "Layer 3",   1 = "Layer 2",   2 = "Layer 1"
 *   3rd index:  bitrate index from frame header
 */
int kbpstab[2][3][16] =
{{
  {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1},
  {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1},
  {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1}
},
 {
   {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1},
   {0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
   {0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160, -1}
 }};

static const unsigned freqtab[9] =
        {44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};

/*
 * returns framesize for the header searchin algoritm
 * 
 */

static int 
get_framesize (unsigned char head[4])
{
  int version=0, freq=0, layer=0, samples=0, kbps, length;

  switch (head[1] & 0x18)
    { /* mpeg version */
    case 0x00:
      version = 2; /* mpeg 2.5 */
      freq = freqtab[6 + ((0x0c & head[2]) >> 2)];
      break;
    case 0x10:
      version = 2; /* mpeg 2 */
      freq = freqtab[3 + ((0x0c & head[2]) >> 2)];
      break;  
    case 0x18:
      version = 1; /* mpeg 1 */
      freq = freqtab[(0x0c & head[2]) >> 2];
      break;
    }
  
  switch (head[1] & 0x06)
    { /* layer */
    case 0x06:
      layer = 1;
      samples = 384;
      break;
    case 0x04:
      layer = 2;
      samples = 1152;
      break;  
    case 0x02:
      layer = 3;
      samples = (version == 1) ? 1152 : 576;
      break; 
    }

  kbps = kbpstab[version - 1][layer - 1][(0xf0 & head[2]) >> 4];
  
  length = (kbps * 125 * samples) / freq;
  
  if (0x02 & head[2])
    {
      if (layer == 1)
        length += 4;
      else
	length++;
    }
  return (length);
}

/*
 * Finds the first header by looking for checks subsequent equal headers...
 * Adds the second byte to the file[filenr] struct
 *
 */

static int
search_first_header (int filenr, int checks)
{
  int pos = -3, pos1, i, count = 0, temp;
  unsigned char head[4], head1[4];
  
  
  if (fseek(file[filenr]->file, 0, SEEK_SET))
    return (-1);
  memset (head, 0, 4);
  
  while (1)
    /* main loop - looks for possible valid headers - exited only by returns */
    {
      fseek (file[filenr]->file, pos + 3, SEEK_SET);
      /* if this fails the next check will catch it */
      if ((temp = getc(file[filenr]->file)) == EOF)
	{
	  if (feof (file[filenr]->file))
	    return (-2);
	  return (-1);
	}
      head[0] = head[1];
      head[1] = head[2];
      head[2] = head[3];
      head[3] = temp;

      if (head[0] == 0xff && is_header(head))
	{
	  count = 1;
	  pos1 = pos;
	  memcpy (head1, head, 4);

	  while (1)
	    {
	      pos1 += get_framesize(head1);
	      fseek (file[filenr]->file, pos1, SEEK_SET);
	      for (i=0; i<4; i++)
		{
		  if ((temp = getc(file[filenr]->file)) == EOF)
		    {
		      if (feof (file[filenr]->file))
			return (-2);
		      return (-1);
		    }
		  head1[i] = temp;
		}
	      if (samestream (head, head1))
		count++;
	      else
		break;
	      if (count == checks)
		{
		  file[filenr]->head = tmalloc(4 * sizeof (unsigned char));
		  memcpy (file[filenr]->head, head, 4);
		  return (pos);
		}
	    }
	}
      pos++;
    }
}

static int
search_next_header (int filenr, unsigned long pos, unsigned int offset)
{
  unsigned char head[4];
  int i, j, temp, shift;

  /* first check if the file has enough bytes left */
  if ((pos + offset + 4) > file[filenr]->filesize)
    return (-1);
  
  /* now check if the head is in the appropriate place */
  /* expect the file to have at least 1 head available, check the getc output later on */
  fseek (file[filenr]->file, pos + offset, SEEK_SET);
  for (i = 0; i < 4; i++)
    head[i] = getc(file[filenr]->file);

  if (samestream (head, file[filenr]->head))
    return (offset); /* success!!! */
   
  /* if not... check 24 next bytes */
  for (i = 1; i < 25; i++)
    {
      if ((temp = getc(file[filenr]->file)) == EOF)
	{
	  if (feof (file[filenr]->file))
	    return (-2);
	  return (-1);
	}
      for (j = 0; j < 3; j++)
	head[j] = head[j + 1];
      head[3] = temp;
      if (samestream(head, file[filenr]->head))
	return (offset + i); /* most common = small shift forward */
    } 
  /* then check right back to pos  */
  /* fill up the head first */
  fseek (file[filenr]->file, pos + offset - 1, SEEK_SET);
  for (i = 0; i < 4; i++)
    head[i] = getc(file[filenr]->file);

  /* now start the backward search */
  fseek (file[filenr]->file, pos + offset, SEEK_SET);
  for (i = 1; i <= offset; i++)
    {
      for (j = 0; j < 3; j++)
	head[j+1] = head [j];
      fseek (file[filenr]->file, -2, SEEK_CUR);
      head[0] = getc (file[filenr]->file);
      if (samestream (head, file[filenr]->head))
	return (offset - i);
    }

  /* then start checking from pos + offset +25 till EOF */
  fseek (file[filenr]->file, pos + offset + 22, SEEK_SET);
  for (i = 0; i < 4; i++)
    head[i] = getc(file[filenr]->file);

  if (samestream (head, file[filenr]->head))
    return (offset + 22); /* success!!! */
   
  /* if not... check 24 next bytes */
  for (i = 1; 1; i++)
    {
      if ((temp = getc(file[filenr]->file)) == EOF)
	{
	  if (feof (file[filenr]->file))
	    return (-2);
	  return (-1);
	}
      for (j = 0; j < 3; j++)
	head[j] = head[j + 1];
      head[3] = temp;
      if (samestream(head, file[filenr]->head))
	return (offset + i); /* most common = small shift forward */
    } 
}

/*
 * parse_static_stream_inf: parses the first header and puts the neccessary
 *                          inf in the stream struct
 */
static void
parse_first_header (stream_t *stream, unsigned char *head)
{
  switch (head[1] & 0x18) {/* mpeg version */
  case 0x00:
    stream->maj_version = 2; /* mpeg 2.5 */
    stream->min_version = 5;
    stream->freq = freqtab[6 + ((0x0c & head[2]) >> 2)];
    break;
  case 0x10:
    stream->maj_version = 2; /* mpeg 2 */
    stream->freq = freqtab[3 + ((0x0c & head[2]) >> 2)];
    break;  
  case 0x18:
    stream->maj_version = 1; /* mpeg 1 */
    stream->freq = freqtab[(0x0c & head[2]) >> 2];
    break;
  }
  
  switch (head[1] & 0x06) {/* layer */
  case 0x06:
    stream->layer = 1;
    stream->samples = 384;
    break;
  case 0x04:
    stream->layer = 2;
    stream->samples = 1152;
    break;  
  case 0x02:
    stream->layer = 3;
    stream->samples = (stream->maj_version == 1) ? 1152 : 576;
    break; 
  }

  stream->mode = ((head[3] & 0xc0) >> 6);
  
  if (stream->maj_version == 1)
    stream->isize = (stream->mode == 3) ? 17 : 32;
  else
    stream->isize = (stream->mode == 3) ? 9 : 17;
  
  stream->cbr = 1;
  stream->crc = !(head[1] & 0x01);
  stream->private = (head[2] & 0x01);
  stream->copyright = (head[3] & 0x08) >> 3;
  stream->original = (head[3] & 0x04) >> 2;
}

/*
 * parse_header: Parses a header for the variable inf.
 *
 */
static void
parse_header (stream_t *stream, header_t *head)
{
  head->kbps = kbpstab[stream->maj_version - 1][stream->layer - 1][(0xf0 & head->head[0]) >> 4];
  
  if (stream->count == 0)
    stream->avkbps = head->kbps;
  else if (!stream->cbr)
    stream->avkbps += (head->kbps - stream->avkbps) / (stream->count + 1);
  else if (stream->cbr && (stream->avkbps != head->kbps))
    {
      stream->cbr = 0;
      stream->avkbps += (head->kbps - stream->avkbps) / (stream->count + 1);
    }
  head->length = (head->kbps * 125 * stream->samples) / stream->freq;
  
  if (0x02 & head->head[0])
    {
      if (stream->layer == 1)
	head->length += 4;
      else
	head->length++;
    }
}

/*
 * read_sideinfo_1:
 *
 */
static void
read_sideinfo_1 (stream_t *stream, info1_t *sideinfo)
{
  int gr0_ch0_size = 0, gr0_ch1_size = 0, gr1_ch0_size = 0, gr1_ch1_size = 0;
  int total, temp;

  sideinfo->backref = sideinfo->info[0] << 1 | sideinfo->info[1] >> 7;
  fprintf (stderr, "backref: %d\n", sideinfo->backref);  
  /* calculate dsize */
  if (stream->isize == 32) /* mpeg 1 stereo */
    {
      gr0_ch0_size = ((sideinfo->info[2] & 0x0f) << 8) | sideinfo->info[3];
      gr0_ch1_size = ((sideinfo->info[9] & 0x01) << 11)
	| (sideinfo->info[10] << 3) | (sideinfo->info[11] >> 5);
      gr1_ch0_size = ((sideinfo->info[17] & 0x3f) << 6) |
	(sideinfo->info[18] >> 2);
      gr1_ch1_size = ((sideinfo->info[24] & 0x07) << 9) |
	(sideinfo->info[25] << 1) | (sideinfo->info[26] >> 7);
    } 
  else /* sisize == 17 - mpeg 1 mono */
    {
      gr0_ch0_size = ((sideinfo->info[2] & 0x1f) << 7) |
	(sideinfo->info[3] >> 1);
      gr1_ch0_size = ((sideinfo->info[9] & 0x03) << 10) |
	(sideinfo->info[10] << 2) | (sideinfo->info[11] >> 6);
    }
  
  total = (gr0_ch0_size + gr0_ch1_size + gr1_ch0_size + gr1_ch1_size) / 8;
  if ((temp = (gr0_ch0_size + gr0_ch1_size + gr1_ch0_size + gr1_ch1_size) % 8))
    total++;
  
  sideinfo->dsize = total;
}

/*
 * read_sideinfo_2:
 *
 */
static void
read_sideinfo_2 (stream_t *stream, info2_t *sideinfo)
{
  int ch0_size = 0, ch1_size = 0;
  int total, temp;

  sideinfo->backref = sideinfo->info[0];
  
  /* calculate dsize */
  if (stream->isize == 9) /* mpeg 2 mono */
    ch0_size = ((sideinfo->info[1] & 0x7f) << 5) | (sideinfo->info[2] >> 3);
  else /* sisize == 17 - mpeg 2 stereo */
    {
      ch0_size = ((sideinfo->info[1] & 0x3f) << 4) |
	(sideinfo->info[2] >> 4);
      ch1_size = ((sideinfo->info[9] & 0x7f) << 5) | (sideinfo->info[10] >> 3);
    }
  
  total = (ch0_size + ch1_size) / 8;
  if ((temp = (ch0_size + ch1_size) % 8))
    total++;
  
  sideinfo->dsize = total;
}
/*
 * first_pass
 * takes a first run through the mp3... places headers in the blocklist.
 *
 */ 
int
first_pass (stream_t *stream, int filenr)
{
  int temp, count = -1;
  unsigned long pos = 0;
  block_t *hblock, *iblock, *block;
  block_t **list;
  
  stream = init_stream ();
  list = stream->list

  if ((temp = search_first_header (filenr, 3)) < 0)
    return (temp);
  parse_first_header (stream, file[filenr]->head);
  stream->head1 = file[filenr]->head[1];

  if (temp > 0)
    add_block (list, &count, filenr, &pos, temp, PADDING);

  while (pos + 128 < file[filenr]->size)
    {

      hblock = add_block (list, &count, filenr, &pos, 4, HEADER);
      ((header_t *) hblock->data)->head = mp3_read (filenr, pos - 2, 2);
      parse_header (stream, hblock->data);

      iblock = add_block (list, &count, filenr, &pos, stream->isize, INF01);
      ((info1_t *) iblock->data)->info = mp3_read (filenr,
						   pos - stream->isize,
						   iblock->size);
      read_sideinfo_1 (stream, iblock->data);

      block = add_block (list, &count, filenr, &pos, 
			 hblock->data->length - 4 - stream->isize, PADDING);
    }
  stream->count = count;
  fprintf (stderr, "Read %d blocks\n", count);
  return (0);
}

/*
 * open_stream: 
 *
 */
int
open_stream (stream_t *stream, int filenr)
{
  int temp, count = -1;
  unsigned long pos = 0;
  block_t *hblock, *iblock, *block;
  block_t **list;
  
  stream = init_stream ();
  list = stream->list

  if ((temp = search_first_header (filenr, 3)) < 0)
    return (temp);
  parse_first_header (stream, file[filenr]->head);
  stream->head1 = file[filenr]->head[1];

  if (temp > 0)
    /* Theres crap in front of the first header*/
    add_block (list, &count, filenr, &pos, temp, PADDING);

  while (pos + 128 < file[filenr]->size)
    /* dirty hack  implement samestream header search */
    {
      /* header */
      hblock = add_block (list, &count, filenr, &pos, 4, HEADER);
      ((header_t *) hblock->data)->head = mp3_read (filenr, pos - 2, 2);
      parse_header (stream, hblock->data);

      /* sideinfo */
      iblock = add_block (list, &count, filenr, &pos, stream->isize, INF01);
      ((info1_t *) iblock->data)->info = mp3_read (filenr,
						   pos - stream->isize,
						   iblock->size);
      read_sideinfo_1 (stream, iblock->data);

      /* padding - filled in later */
      block = add_block (list, &count, filenr, &pos, 
			 hblock->data->length - 4 - stream->isize, PADDING);
      
      if (((info1_t *) iblock->data)->dsize)
	{
	  /* Lets find out where the data is shall we */

	  int dsize = ((info1_t *) iblock->data)->dsize;
	  int backref = ((info1_t *) iblock->data)->backref;
	  int offset = backref - dsize;
	  unsigned char type = DATA;
	  int count2 = count;
	  block_t *dblock = NULL;

	  if (offset > 0)
	    {
	      if (offset > block->size)
		/* the data cannot go beyond the next frame header */
		{
		 fprintf (stderr, "BAD stream formatting,"
			       "size overflow\n"); 
		 block->type = type = BAD;
		 block->data = new_data (NULL, hblock);
		 dsize = backref;
		}
	      else
		{
		  if (offset < block->size)
		    block = split_block (list, count, count, offset);
		  block->type = DATA;
		  block->data = new_data (NULL, hblock);
		  dsize -= offset;
		}
	      offset = 0;
	    }

	  while (dsize > 0) /* goto where the new data should begin */
	    {
	      if (count2 > 0)
		{
		  int size;
		  count2--;
		  block = list[count2];
		  size = block->size;

		  switch (block->type)
		    {
		    case HEADER:
		    case INF1:
		    case INF2:
		      break; 
		    case PADDING:
		      if (offset)
			{
			  if (-offset < size)
			    block = split_block (list, &count, &count2,
						 size + offset); 
			  offset -= size;
			}
		      else
			{
			  if (size > dsize)
			    {
			      block = split_block (list, &count, &count2,
						   size - dsize);
			      dsize = 0;
			    }
			  else
			    dsize -= size;
			  
			  block->type = type;
			  if (dblock)
			    {
			      if (type > 0xf0)
				memcpy (block->data, dblock->data,
					sizeof (overlap_t));
			      else
				memcpy (block->data, dblock->data,
					sizeof (data_t));
			    }
			  block->data->next = dblock;
			  dblock = block;
			}
		      break;
		    case DATA:
		    case OVERLAPPING:
		    case BAD:
		      if (offset)
			{
			  if (-offset < size)
			    {
			      split_block (list, *count, count2,
					   size + offset); 
			      count2++;
			    }
			  offset -= size;
			}
		      else
			{
			  
			  
			}
		      type = OVERLAPPING;
		      fprintf (stderr, "BAD stream formatting,"
			       "overlapping data\n");
		      break;
		      
		    case OVERLAP:
		    case BAD_OVERLAP:
		      fprintf(stderr, "WTF? Multiple overlapped files???"
			      "omg!");
		    default:
		      break;
		    }
		}
	      else
		{
		  dsize -= backref;
		  backref = 0;
		  fprintf (stderr, "BAD stream formatting, first frame"
			   "has a backref! Invalidating data!\n");
		  type = BAD;
		  while ((list[count2]->type == HEADER) ||
			 (list[count2]->type == INF1) ||
			 (list[count2]->type == INF2))
		    count++;
		}
	    }
	  block = list[count2];
	  if (type == OVERLAPPING)
	    {
	  switch (type)
	    {
	    case OVERLAPPING:
	      /* first, set the other datablocks to overlapping */
	      dblock = block->data->headp->data->datap;
	      while (dblock)
		{
		  dblock->type = OVERLAPPING;
		  print_block (dblock);
		  dblock = dblock->next;
		}
	      /* this is the first frame of the overlapping
		 so make new block that is the actual overlap */
	      if (backref < 0) /* not on a frame boundary */
		{

		  count++;
		  stream->list = trealloc (stream->list, (count + 1)*sizeof(block_t *));
		  for (i = count - 1; i > count2; i--)
		    stream->list[i + 1] = stream->list[i];
		  dblock = stream->list[++count2] = tmalloc(sizeof(block_t));
		  dblock->type = OVERLAPPED;
		  dblock->filenr = filenr;
		  dblock->pos = block->pos - backref;
		  
		  dblock->size = block->size + backref;
		  block->size = - backref;
		  
		  dblock->data = tmalloc (sizeof(overlap_t));
		  dblock->data->next = block->data->next;
		  block->data->next = dblock;
		  print_block (block);
		  dblock->data->head1p = block->data->headp;
		  dblock->data->head2p = hblock;
		  hblock->data->datap = dblock;
		  block = dblock;
		  backref = 0;
		}
	      else /* can only be backref == 0 | on a block boundary */
		{
		  void *temp;
		  block->type = OVERLAPPED;
		  temp = block->data;
		  block->data = tmalloc (sizeof(overlap_t));
		  block->data->next = temp->next;
		  block->data->head1p = temp->headp;
		  block->data->head2p = hblock;
		  free (temp);
		}
	      dsize -= block->size;
	      
	      /* check if there r more datablocks overlapped
	       * normally this shouldnt be run at all, or only once
	       * but in the highly unlikely case of that actually happening... */
	      while (block->type != PADDING)
		{
		  switch (block->type)
		    {
		    case DATA:
		      
		      while (block->data->next)
			{
			  void *temp;
			  block = block->data->next;
			  temp = block->data;
			  block->type = OVERLAPPED;
			  block->data = tmalloc(sizeof(overlap_t));
			  block->data->next = temp->next;
			  block->data->head1p = temp->headp;
			  block->data->head2p = hblock;
			  free (temp);
			  dsize -= block->size;
			}
		  block = stream->list[++count2];
		}
	      /* now find the next padding, or if theres still some data
		 from yet another frame there */
	      /* find out where the block is */
	      count2 = count;
	      while (block != stream->list[count2])
		count2--;
	      block = stream->list[++count2];
	      while (block->type != PADDING)




	      break;
	    case BAD:
	      /* only happens when first frame has a backref
	       so type set to BAD will suffice */
	      break;
	    case DATA:
	      /* datablocks rnt adjacent, so padding is in between */
	      if (backref < 0)
		{
		  int i;	  
		  count++;
		  stream->list = trealloc (stream->list, (count + 1)*sizeof(block_t *));
		  for (i = count - 1; i > count2; i--)
		    stream->list[i + 1] = stream->list[i];
		  dblock = stream->list[count2 + 1] = new_block (filenr, block->pos - backref, block->size + backref, PADDING);
		  block->size = - backref;
		  block = dblock;
		  backref = 0;
		  dblock = NULL;
		}
	      break;
	    }
	  while (1) 
	    {
	      block->type = type;
	      block->data = tmalloc(sizeof(data_t));
	      ((data_t *) block->data)->headp = hblock;
	      ((data_t *) block->data)->next = NULL;
	      if (dblock)
		((data_t *) dblock->data)->next = block;
	      else
		((header_t *) hblock->data)->datap = block;
	      dblock = block;
	      dsize -= block->size;
	      if (dsize == 0)
		break;
	      if (dsize < 0)
		/* shift higher blocks and insert a new PADDING block */
		{
		  int i;
		  count++;
		  stream->list = trealloc (stream->list, (count + 1)*sizeof(block_t *));
		  for (i = count - 1; i > count2; i--)
		    stream->list[i + 1] = stream->list[i];
		  dblock->size += dsize;
		  dblock = stream->list[count2 + 1] = new_block(filenr, block->pos + block->size, -dsize, PADDING);
		  dsize = 0;
		  break;
		}
	      if (count2 == count)
		{
		  if (dsize > 0)
		    fprintf(stderr, "Bad stream formatting, frame data overflow\n");
		  break; /* add code here */
		}
	      fprintf (stderr, "count: %d\n", count);
	      block = stream->list[count2 += 3];
	      if (block->type != PADDING) /* shouldnt happen */
		fprintf(stderr, "Bad stream formatting, block %d shouldve been padding!\n", count2);
	    }
	  stream->framecount++;
	}
      count++;
    }
  stream->count = count;
  fprintf (stderr, "Read %d blocks\n", count);
  return (0);
}

/* EOF */
