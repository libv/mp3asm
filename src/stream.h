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

#ifndef HAVE_STREAM_H
#define HAVE_STREAM_H

#include <glib.h>

/* block_t->type */
#define PADDING 0x00
#define HEADER 0x01
#define SIDEINFO1 0x02
#define SIDEINFO2 0x03
#define DATA 0x04
#define ID3V1 0x05
#define ID3V2 0x06
#define BAD 0xff

typedef struct block_t
{
  unsigned char filenr;
  long pos;
  guint16 size;
  unsigned char type;
  void *data;
} block_t;

/* type = PADDING; no *data */

/* type = HEADER */
typedef struct header_t
{
  unsigned char kbps; /* multiply by 8 to get kbps*/
  guint16 length; /* space from header to header */
  unsigned char *head; /* actual header - first byte should be 0xff
			    second byte = static stream info */
  block_t *datap; /* place in data struct */
} header_t;

/* type = SIDEINFO1 */
typedef struct info1_t
{
  guint16 backref; /* actual backref in the sideinfo */
  guint16 dsize; /* actual size of the data */ 
  guint16 gain[4]; /* global gain in info */
  unsigned char *info; /* 17-32 bytes of sideinfo */
} info1_t;

/* type = SIDEINFO2 */
typedef struct info2_t
{
  guint8 backref; /* actual backref in the sideinfo */
  guint16 dsize; /* actual size of the data */ 
  guint16 gain[2]; /* global gain in info */
  unsigned char *info; /* 9-17 bytes of sideinfo */
} info2_t;

/* type = DATA */
typedef struct data_t
{
  block_t *next;  /* place in data struct */
  block_t *headp; /* place in data struct */
} data_t;

/* type = ID3V1 */
typedef struct id3v1_t
{
  unsigned char tag[128];
} id3v1_t;

typedef struct stream_t
{
  int maj_version;  /* 1, 2 */
  int min_version;  /* 5 -> 2.5 */
  int layer; /* 1, 2, 3 */
  int samples; /* per frame */
  int isize; /* side info size */
  int cbr; /* boolean */
  float avkbps;
  int freq;
  int mode; /* 0 = stereo | 1 = joint stereo | 2 = dual chan | 3 = mono */
  int crc;  
  int private;
  int copyright;
  int original;
  unsigned char head1;
  
  long count;
  long framecount;

  block_t **list; /* pointer to first frame of the stream */
} stream_t;

#endif /* HAVE_STREAM_H */

/* EOF */
