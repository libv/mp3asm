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

#ifndef HAVE_MP3_BLOCK_H
#define HAVE_MP3_BLOCK_H

#include <glib.h>

/* Ok we need to know the
   type (2nd half of a unsigned char)
       padding   0x0 - 0000
       head/info 0x8 - 1000
           v1    0x8 - 1000
	   v2    0x9 - 1001
       data      0xa - 1010
       id3       0xc - 1100
           v1    0xc - 1100
	   v2    0xd - 1101
       xing vbr  0xe - 1110
    further explanation
      bits  1: padding/not padding
          2/3: type of data present
	    4: further definition

   errors
       yes/no       0x80   100.00.00.0
       checked/not         110.00.00.0   
       repaired/not        111.00.00.0
   offset - headers r the shifted
       yes/no              1??.10.00.0
       local/not           1??.11.00.0
   overlap                 
       yes/no              1??.??.10.?
       local/not           1??.??.11.?
   bad audio
       yes/no              1??.??.??.1
*/

/* now, lets define mp3_block_t->(unsigned char type) */

#define PAD       0x00
#define MAIN      0xf8
#define NOT_PAD   0x80
#define TYPE      0x60
#define SEC       0x10
#define V1        0x00
#define V2        0x10
#define THI       0x00
#define MONO      0x00
#define STEREO    0x08

#define HEAD      0x00
#define DATA      0x20
#define ID3       0x40
#define XVBR      0x60

/* mp3_block_t->(unsigned char error) */
#define CORR      0x00
#define ERR       0x80
#define CHECK     0x40
#define REP       0x20
#define SHIFT     0x10
#define SHIFT_LOC 0x08
#define LAP       0x04
#define LAP_LOC   0x02
#define BAD       0x01

typedef struct mp3_block_t
{
  unsigned char filenr;
  long pos;
  guint16 size;
  char type;
  char error;
  void *data;
} block_t;

/* type = PADDING; no *data */

/* type = HEAD11 v1 layer 3 mono */
typedef struct head11_t
{
  guint16 length; /* space from header to header */
  guint16 backref; /* actual backref in the sideinfo */
  guint16 dsize; /* actual size of the data */ 
  guint16 gain[2]; /* global gain in info */
  unsigned char head[2]; /* actual header - first byte should be 0xff
			    second byte = static stream info */
  unsigned char info[17]; /* 17 bytes of sideinfo */
  block_t *datap; /* place in data struct */
} head11_t;

/* type = HEAD11 v1 layer 3 stereo */
typedef struct head12_t
{
  guint16 length; 
  guint16 backref; 
  guint16 dsize; 
  guint16 gain[4]; 
  unsigned char head[2];
  unsigned char info[32];
  block_t *datap;
} head12_t

/* type = HEAD11 v2 layer 3 mono */
typedef struct head21_t
{
  guint16 length;
  guint8  backref;
  guint16 dsize;
  guint16 gain[1];
  unsigned char head[2];
  unsigned char info[9];
  block_t *datap;
} head21_t;

/* type = HEAD11 v2 layer 3 stereo */
typedef struct head22_t
{
  guint16 length;
  guint8  backref;
  guint16 dsize;
  guint16 gain[2];
  unsigned char head[2];
  unsigned char info[17];
  block_t *datap;
} head22_t;

/* type = DATA/OVERLAPPING/BAD */
typedef struct data_t
{
  block_t *next;  
  block_t *headp; 
} data_t;

/* type = OVERLAP/BAD_OVERLAPPED */
typedef struct overlap_t
{
  block_t *next;  
  block_t *head1p;
  block_t *head2p;
} overlap_t;

/* type = ID3V1 */
typedef struct id3v1_t
{
  unsigned char tag[128];
} id3v1_t;

/* type = ID3V2 */
typedef struct id3v2_t
{
} id3v2_t;

/* type = XVBR */
typedef struct xvbr_t
{
} xvbr_t;

#endif /* HAVE_MP3_BLOCK_H */

/* EOF */
