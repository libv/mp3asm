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
 * search header: searches for 3 simmilar possible headers
 *                writes the header to the file struct & returns 0
 *                
 *  
 */
/* should rewrite it to search for a header, read the framelength & then get
 * the next header */
int
search_first_header (int filenr)
{
  int i = -3, count = 0, k, temp;
  unsigned char head[4];
  
  typedef struct header_t
  {
    unsigned char head[4];
    int count;
    int pos;
  } header_t;
  
  header_t *heads = NULL;
  
  if (fseek(file[filenr]->file, 0, SEEK_SET))
    return (-1);;
  memset (head, 0, 4);
  
  while (1)
    switch (temp = getc(file[filenr]->file))
      {
      case EOF:
	if (feof (file[filenr]->file))
	  return (-2);
	return (-1);
      default:
	head[0] = head[1];
	head[1] = head[2];
	head[2] = head[3];
	head[3] = temp;
	if (head[0] == 0xff && is_header(head))
	  {
	    for (k = 0; k < count; k++)
	      if (samestream (head, heads[k].head))
		{
		  if (heads[k].count == 3)
		    {
		      file[filenr]->head = tmalloc(4 * sizeof (unsigned char));
		      memcpy (file[filenr]->head, heads[k].head, 4);
		      temp = heads[k].pos;
		      free (heads);
		      return (temp);
		    }
		  heads[k].count++;
		  break;
		}
	    if (k == count)
	      {
		count++;
		heads = trealloc (heads, count * sizeof (header_t));
		memcpy (heads[k].head, head, 4);
		heads[k].count = 1;
		heads[k].pos = i;
	      }
	  }
	i++;
      }
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
 * parse_static_stream_inf: parses the first header and puts the neccessary
 *                          inf in the stream struct
 */
static void
parse_first_header (stream_t *stream,unsigned char *head)
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
  /* fprintf (stderr, "backref: %d\n", sideinfo->backref); */
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
 * open_stream: 
 *
 */
int
open_stream (stream_t *stream, int filenr)
{
  int temp, count = 0;
  unsigned long pos = 0;
  block_t *hblock, *iblock, *block;
  
  stream = init_stream ();
  if ((temp = search_first_header (filenr)) < 0)
    return (temp);
  parse_first_header (stream, file[filenr]->head);
  stream->head1 = file[filenr]->head[1];

  if (temp > 0) /* skip the stuff in front of the frame, mark it as padding */
    {
      fprintf (stderr, "temp = %d\n", temp);
      stream->list = tmalloc (sizeof(block_t *)); /* prep for first block */
      block = stream->list[0] = tmalloc (sizeof(block_t));
      
      block->filenr = filenr;
      block->pos = 0;
      block->size = temp;
      block->type = PADDING;
      block->data = NULL;
      count++; /* prep for header */
      pos = temp;
    }
  while (pos + 128 < file[filenr]->size) /* dirty hack  implement samestream header search */
    {
      stream->list = trealloc(stream->list, (count + 1)*sizeof(block_t *));
      hblock = stream->list[count] = tmalloc (sizeof(block_t));
      hblock->filenr = filenr;
      hblock->pos = pos;
      hblock->type = HEADER;
      hblock->size = 4;
      hblock->data = tmalloc(sizeof(header_t));
      pos += 2; /* implement check here!!! */
      ((header_t *) hblock->data)->head = mp3_read (filenr, pos, 2);
      parse_header (stream, hblock->data);
      pos += 2;
      /* next block, sideinf */
      count++;
      stream->list = trealloc(stream->list, (count + 1)*sizeof(block_t *));
      iblock = stream->list[count] = tmalloc (sizeof(block_t));
      iblock->filenr = filenr;
      iblock->pos = pos;
      iblock->type = SIDEINFO1;
      iblock->size = stream->isize;
      iblock->data = tmalloc (sizeof(info1_t));
      ((info1_t *) iblock->data)->info = mp3_read (filenr, pos, iblock->size);
      read_sideinfo_1 (stream, iblock->data);
      pos += stream->isize;
      /* put in padding, so that it can be filled later */
      count++;
      stream->list = trealloc(stream->list, (count + 1)*sizeof(block_t *));
      block = stream->list[count] = tmalloc(sizeof(block_t));
      block->filenr = filenr;
      block->pos = pos;
      block->type = PADDING;
      block->size = ((header_t *) hblock->data)->length - 4 - stream->isize;
      block->data = NULL;
      pos = block->pos + block->size;
      
      /* find out where the data is */
      if (((info1_t *) iblock->data)->dsize)
	{
	  int dsize = ((info1_t *) iblock->data)->dsize;
	  int backref = ((info1_t *) iblock->data)->backref;
	  int count2 = count;
	  block_t *datablock = NULL;
	  while (backref > 0) /* goto where the first data block should be */
	    {
	      count2 -= 3;
	      block = stream->list[count2];
	      if (block->type != PADDING) /* write up some code here */
		fprintf (stderr, "BAD stream formatting, bad backref\n");
	      backref -= block->size;
	    }
	  if (backref < 0)
	    {
	      int i;	  
	      count++;
	      stream->list = trealloc (stream->list, count*sizeof(block_t *));
	      for (i = count - 1; i > count2; i--)
		stream->list[i + 1] = stream->list[i];
	      datablock = stream->list[count2 + 1] = tmalloc(sizeof(block_t));
	      datablock->type = PADDING;
	      datablock->filenr = filenr;
	      datablock->pos = block->pos - backref;
	      datablock->size = block->size + backref;
	      block->size = - backref;
	      datablock->data = NULL;
	      block = datablock;
	      backref = 0;
	      datablock = NULL;
	    }
	  while (1) 
	    {
	      block->type = DATA;
	      block->data = tmalloc(sizeof(data_t));
	      ((data_t *) block->data)->headp = hblock;
	      ((data_t *) block->data)->next = NULL;
	      if (datablock)
		((data_t *) datablock->data)->next = block;
	      else
		((header_t *) hblock->data)->datap = block;
	      datablock = block;
	      dsize -= block->size;
	      if (dsize < 0)
		/* shift higher blocks and insert a new PADDING block */
		{
		  int i;
		  
		  count++;
		  stream->list = trealloc (stream->list, count*sizeof(block_t *));
		  for (i = count - 1; i > count2; i--)
		    stream->list[i + 1] = stream->list[i];
		  datablock->size += dsize;
		  datablock = stream->list[count2 + 1] = tmalloc(sizeof(block_t));
		  datablock->type = PADDING;
		  datablock->filenr = filenr;
		  datablock->pos = block->pos + block->size;
		  datablock->size = -dsize;
		  datablock->data = NULL;
		  dsize = 0;
		  break;
		}
	      if (count2 == count)
		{
		  if (dsize > 0)
		    fprintf(stderr, "Bad stream formatting, frame data overflow\n");
		  break; /* add code here */
		}
	      block = stream->list[count2 += 3];
	      if (block->type != PADDING)
		fprintf(stderr, "Bad stream formatting, frame %d shouldve been padding!\n", count2);
	    }
	  stream->framecount++;
	  fprintf (stderr, "framecount: %d\n", stream->framecount);
	}
      count++;
    }
  stream->count = count;
  fprintf (stderr, "Read %d blocks\n", count);
  return (0);
}

/* EOF */
