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

#ifndef HAVE_INPUT_H
#define HAVE_INPUT_H

#include <stdio.h>
#include "commandline.h"

#define CHUNK_EMPTY  0 // not assigned yet.
#define CHUNK_HEAD   1 // includes sideinfo in l3 and mpeg 2/2.5
#define CHUNK_DATA   2 // data, usually several strips of it
#define CHUNK_ANCIL  3 // ancillary data, to be copied over
#define CHUNK_ERROR  4 // anything else

struct chunk_t {
  // type: 4 bits should suffice; see defines higher up
  // subcount: 8 bits should be plenty: ie.: data can be in several chunks
  // size: 16 bits: size of this chunk
  // keep: dont delete this just yet.
  unsigned int type:4, subcount:8, size:16, keep:1;
  unsigned int count; // framecount
  unsigned int position; // position in stream of start of chunk
};

#define DEFAULT_INPUT_BUFFER_SIZE 51200 // 50 KiloByte, enough for 50 320kbps frames.
struct input_buffer_t {
  // old data that wasnt segmented yet should be moved to the front
  unsigned char *buffer; // actual data buffer
  unsigned int size;

  unsigned int position; // position in stream of start of block
  unsigned int used; // keep track of what is actually in this buffer
};

// the number of headers that must be found for a stream to be valid
#define DEFAULT_INPUT_HEADER_TEST 30

// add in CRC
struct input_file_info_t {
  unsigned long header; // always 32 bit
  unsigned int header_position;
  unsigned int bitrate; // 4 bit
  unsigned int frame_count; // to calculate the vbr avg
  unsigned int vbr; // 1 bit

  unsigned int version; //2b 0: mpeg 1; 1: mpeg 2; 2: mpeg 2.5
  unsigned int layer; //0: l1; 1: l2; 2: l3
  unsigned int frequency; // 0;1;2
  
  unsigned int *size_table_normal;
  unsigned int *size_table_padded;

  unsigned int info_size; //8b should do
  unsigned int (*info_read_backref) (unsigned int position);
  unsigned int (*info_read_datasize) (unsigned int position);
};

struct input_t {
  struct commandline_file_t *file;

  FILE *stream;

  struct input_buffer_t buffer[1];

  struct input_file_info_t info[1];

  unsigned int chunk_count;
  struct chunk_t **chunks;
};

struct input_t input[1];

#endif /* HAVE_INPUT_H */
