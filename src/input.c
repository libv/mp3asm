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
#include <errno.h>
#include <string.h>
#include "tools.h"
#include "input.h"
#include "logging.h"
#define DEBUG

#ifdef DEBUG
#include <stdlib.h>
#endif

static unsigned int
buffer_position (unsigned int position)
{
#ifdef DEBUG
  if (position < input->buffer->position)
    exit (0);
#endif
  return position - input->buffer->position;

}

#define BP(x) buffer_position (x)

struct input_t input[1];

/*
 *
 */
void
input_init (void)
{
  // set the basic input structure
  input->file = commandline.inputs[0];
  
  // open the file later.
  input->stream = NULL;

  // buffer itself doesnt need to be memset 0
  input->buffer->size = DEFAULT_INPUT_BUFFER_SIZE;
  input->buffer->buffer = nmalloc (input->buffer->size * sizeof (unsigned char));
  input->buffer->position = 0;
  input->buffer->used = 0;

  memset (input->info, 0, sizeof(struct input_file_info_t));

  input->chunks = NULL;
  input->chunk_count = 0;
}

/*
 *
 */
int
input_open (void)
{
  if (input->file->filename) {
    input->stream = fopen (input->file->filename, "r");
    if (!input->stream) {
      log_once (LOG_QUIET, "Error: Unable to open inputfile \"%s\": %s", input->file->filename, strerror (errno));
      return -1;
    }
  } else {
    input->stream = stdin;
  }
  return 0;
}

/*
 *
 * return:
 *  -1: error has occured, exit as clean as possible.
 *   0: data present, handle normally.
 *   1: eof: data present, clean up and close this file, open next.
 */
int 
input_read (void)
{
  int bytes_read = fread (input->buffer->buffer + input->buffer->used, 1,
			  input->buffer->size - input->buffer->used, input->stream);
  if (bytes_read < (input->buffer->size - input->buffer->used)) {
    if (feof (input->stream))
      return 1;
    else if (ferror) {
      log_once (LOG_QUIET, "Error: Unable to read from inputfile \"%s\": %s", input->file->filename, strerror (ferror(input->stream)));
      return -1;
    }
  }
  return 0;
}

/*
 *
 */
static int
header_valid (unsigned int position)
{
  if (input->buffer->buffer[position] == 0xff) { // framesync
    char *p = input->buffer->buffer + position + 1;
    if (((*p & 0xe0) != 0xe0) || ((*p & 0x18) == 0x08) || !(*p & 0x06)) 
      // framesync, mpeg version reserved, layer reserved.
      return 1;
    p++;
    if (((*p & 0xf0) == 0xf0) || ((*p & 0x0c) == 0x0c))
      return 1;
    log_once (LOG_QUIET, "possible header: %8d: %02x%02x%02x%02x", position, input->buffer->buffer[position],
	      input->buffer->buffer[position + 1],input->buffer->buffer[position + 2], 
	      input->buffer->buffer[position + 3]);
    return 0;
  }
  return 1;
}

/*
 *
 */
static int
header_compatible (unsigned int position)
{
  unsigned char *p = input->buffer->buffer + position;
  // 0x000cc300
  unsigned int test = ((*p << 24) | (*(p + 1) << 16) | (*(p + 2) << 8) | (*(p + 3)));

  //log_once (LOG_QUIET, "test %d: %8x <-> %8x", position, input->info->header, test);
  if (input->info->header == (((*p << 24) | (*(p + 1) << 16) | (*(p + 2) << 8) | (*(p + 3))) & 0xffff0dcf))
    return 0;
  return 1;
}

static unsigned int
sideinfo_read_backref_v1_mono (unsigned int position)
{
  
  
  return 0;
}

static unsigned int
sideinfo_read_backref_v1_stereo (unsigned int position) // i cant name a function "everything_but_mono"
{
  //log_once (LOG_QUIET, "sideinfo: %02x%02x", input->buffer->buffer[position + 4], input->buffer->buffer[position + 5]);
  return ((input->buffer->buffer[position + 4] << 1) | (input->buffer->buffer[position + 5] >> 7));
}

static unsigned int
sideinfo_read_backref_v2_mono (unsigned int position) 
// i know, this one is much of the same, "everything but v1" is not an option
{



  return 0;
}

static unsigned int
sideinfo_read_backref_v2_stereo (unsigned int position)
{

  return 0;
}

static unsigned int
sideinfo_read_datasize_v1_mono (unsigned int position)
{
  
  
  return 0;
}

static unsigned int
sideinfo_read_datasize_v1_stereo (unsigned int position) // i cant name a function "everything_but_mono"
{
  unsigned char *buf = input->buffer->buffer + position + 4;
  unsigned int size = 0;
  size += ((buf[2] & 0x0f) << 8) | buf[3];
  size += ((buf[9] & 0x01) << 11) | (buf[10] << 3) | (buf[11] >> 5);
  size += ((buf[17] & 0x3f) << 6) | (buf[18] >> 2);
  size += ((buf[24] & 0x07) << 9) | (buf[25] << 1) | (buf[26] >> 7);

  if (size % 8)
    return (size / 8) + 1;
  return (size / 8);
    
}

static unsigned int
sideinfo_read_datasize_v2_mono (unsigned int position) 
// i know, this one is much of the same, "everything but v1" is not an option
{



  return 0;
}

static unsigned int
sideinfo_read_datasize_v2_stereo (unsigned int position)
{

  return 0;
}

/*
 *
 */
static void
header_parse (void)
{
  /* int kbps[2][3][16] = {{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1},
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1},
    {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1}
  },{
    {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1},
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1},
    {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1}}};
  int frequencies[3][3] = {{44100, 48000, 32000}, 
			   {22050, 24000, 16000}, 
			   {11025, 12000,  8000}}; */
  unsigned int sizes[3][3][3][2][15] = // version, layer, frequency, padding, bitrate
    {{{{{0,   34,   69,  104,  139,  174,  208,  243,  278,  313,  348,  383,  417,  452,  487},    //v1   l1 f0 p0
	{0,   38,   73,  108,  143,  178,  212,  247,  282,  317,  352,  387,  421,  456,  491}},   //v1   l1 f0 p4 
       {{0,   32,   64,   96,  128,  160,  192,  224,  256,  288,  320,  352,  384,  416,  448},    //v1   l1 f1 p0
	{0,   36,   68,  100,  132,  164,  196,  228,  260,  292,  324,  356,  388,  420,  452}},   //v1   l1 f1 p4
       {{0,   48,   96,  144,  192,  240,  288,  336,  384,  432,  480,  528,  576,  624,  672},    //v1   l1 f2 p0 
	{0,   52,  100,  148,  196,  244,  292,  340,  388,  436,  484,  532,  580,  628,  676}}},  //v1   l1 f2 p4
      {{{0,   52,   78,   91,  104,  130,  156,  182,  208,  261,  313,  365,  417,  522,  626},    //v1   l2 f0 p0
	{0,   53,   79,   92,  105,  131,  157,  183,  209,  262,  314,  366,  418,  523,  627}},   //v1   l2 f0 p1
       {{0,   48,   72,   84,   96,  120,  144,  168,  192,  240,  288,  336,  384,  480,  576},    //v1   l2 f1 p0
	{0,   49,   73,   85,   97,  121,  145,  169,  193,  241,  289,  337,  385,  481,  577}},   //v1   l2 f1 p1
       {{0,   72,  108,  126,  144,  180,  216,  252,  288,  360,  432,  504,  576,  720,  864},    //v1   l2 f2 p0 
	{0,   73,  109,  127,  145,  181,  217,  253,  289,  361,  433,  505,  577,  721,  865}}},  //v1   l2 f2 p1
      {{{0,  104,  130,  156,  182,  208,  261,  313,  365,  417,  522,  626,  731,  835, 1044},    //v1   l3 f0 p0
	{0,  105,  131,  157,  183,  209,  262,  314,  366,  418,  523,  627,  732,  836, 1045}},   //v1   l3 f0 p1
       {{0,   96,  120,  144,  168,  192,  240,  288,  336,  384,  480,  576,  672,  768,  960},    //v1   l3 f1 p0
	{0,   97,  121,  145,  169,  193,  241,  289,  337,  385,  481,  577,  673,  769,  961}},   //v1   l3 f1 p1
       {{0,  144,  180,  216,  252,  288,  360,  432,  504,  576,  720,  864, 1008, 1152, 1440},    //v1   l3 f2 p0
	{0,  145,  181,  217,  253,  289,  361,  433,  505,  577,  721,  865, 1009, 1153, 1441}}}}, //v1   l3 f2 p1
     {{{{0,   69,  139,  208,  278,  348,  417,  487,  557,  626,  696,  766,  835,  905,  975},    //v2   l1 f0 p0
	{0,   73,  143,  212,  282,  352,  421,  491,  561,  630,  700,  770,  839,  909,  979}},   //v2   l1 f0 p4
       {{0,   64,  128,  192,  256,  320,  384,  448,  512,  576,  640,  704,  768,  832,  896},    //v2   l1 f1 p0
	{0,   68,  132,  196,  260,  324,  388,  452,  516,  580,  644,  708,  772,  836,  900}},   //v2   l1 f1 p4
       {{0,   96,  192,  288,  384,  480,  576,  672,  768,  864,  960, 1056, 1152, 1248, 1344},    //v2   l1 f2 p0
	{0,  100,  196,  292,  388,  484,  580,  676,  772,  868,  964, 1060, 1156, 1252, 1348}}},  //v2   l1 f2 p4
      {{{0,  104,  156,  182,  208,  261,  313,  365,  417,  522,  626,  731,  835, 1044, 1253},    //v2   l2 f0 p0
	{0,  105,  157,  183,  209,  262,  314,  366,  418,  523,  627,  732,  836, 1045, 1254}},   //v2   l2 f0 p1
       {{0,   96,  144,  168,  192,  240,  288,  336,  384,  480,  576,  672,  768,  960, 1152},    //v2   l2 f1 p0
	{0,   97,  145,  169,  193,  241,  289,  337,  385,  481,  577,  673,  769,  961, 1153}},   //v2   l2 f1 p1
       {{0,  144,  216,  252,  288,  360,  432,  504,  576,  720,  864, 1008, 1152, 1440, 1728},    //v2   l2 f2 p0
	{0,  145,  217,  253,  289,  361,  433,  505,  577,  721,  865, 1009, 1153, 1441, 1729}}},  //v2   l2 f2 p1
      {{{0,  104,  130,  156,  182,  208,  261,  313,  365,  417,  522,  626,  731,  835, 1044},    //v2   l3 f0 p0
	{0,  105,  131,  157,  183,  209,  262,  314,  366,  418,  523,  627,  732,  836, 1045}},   //v2   l3 f0 p1
       {{0,   96,  120,  144,  168,  192,  240,  288,  336,  384,  480,  576,  672,  768,  960},    //v2   l3 f1 p0
	{0,   97,  121,  145,  169,  193,  241,  289,  337,  385,  481,  577,  673,  769,  961}},   //v2   l3 f1 p1
       {{0,  144,  180,  216,  252,  288,  360,  432,  504,  576,  720,  864, 1008, 1152, 1440},    //v2   l3 f2 p0
	{0,  145,  181,  217,  253,  289,  361,  433,  505,  577,  721,  865, 1009, 1153, 1441}}}}, //v2   l3 f2 p1
     {{{{0,  139,  208,  243,  278,  348,  417,  487,  557,  626,  696,  766,  835,  975, 1114},    //v2.5 l1 f0 p0
	{0,  143,  212,  247,  282,  352,  421,  491,  561,  630,  700,  770,  839,  979, 1118}},   //v2.5 l1 f0 p4
       {{0,  128,  192,  224,  256,  320,  384,  448,  512,  576,  640,  704,  768,  896, 1024},    //v2.5 l1 f1 p0
	{0,  132,  196,  228,  260,  324,  388,  452,  516,  580,  644,  708,  772,  900, 1028}},   //v2.5 l1 f1 p4
       {{0,  192,  288,  336,  384,  480,  576,  672,  768,  864,  960, 1056, 1152, 1344, 1536},    //v2.5 l1 f2 p0
	{0,  196,  292,  340,  388,  484,  580,  676,  772,  868,  964, 1060, 1156, 1348, 1540}}},  //v2.5 l1 f2 p4
      {{{0,   52,  104,  156,  208,  261,  313,  365,  417,  522,  626,  731,  835,  940, 1044},    //v2.5 l2 f0 p0
	{0,   53,  105,  157,  209,  262,  314,  366,  418,  523,  627,  732,  836,  941, 1045}},   //v2.5 l2 f0 p1
       {{0,   48,   96,  144,  192,  240,  288,  336,  384,  480,  576,  672,  768,  864,  960},    //v2.5 l2 f1 p0
	{0,   49,   97,  145,  193,  241,  289,  337,  385,  481,  577,  673,  769,  865,  961}},   //v2.5 l2 f1 p1
       {{0,   72,  144,  216,  288,  360,  432,  504,  576,  720,  864, 1008, 1152, 1296, 1440},    //v2.5 l2 f2 p0
	{0,   73,  145,  217,  289,  361,  433,  505,  577,  721,  865, 1009, 1153, 1297, 1441}}},  //v2.5 l2 f2 p1
      {{{0,   52,  104,  156,  208,  261,  313,  365,  417,  522,  626,  731,  835,  940, 1044},    //v2.5 l3 f0 p0
	{0,   53,  105,  157,  209,  262,  314,  366,  418,  523,  627,  732,  836,  941, 1045}},   //v2.5 l3 f0 p1
       {{0,   48,   96,  144,  192,  240,  288,  336,  384,  480,  576,  672,  768,  864,  960},    //v2.5 l3 f1 p0
	{0,   49,   97,  145,  193,  241,  289,  337,  385,  481,  577,  673,  769,  865,  961}},   //v2.5 l3 f1 p1
       {{0,   72,  144,  216,  288,  360,  432,  504,  576,  720,  864, 1008, 1152, 1296, 1440},    //v2.5 l3 f2 p0
	{0,   73,  145,  217,  289,  361,  433,  505,  577,  721,  865, 1009, 1153, 1297, 1441}}}}};//v2.5 l3 f1 p1 


  input->info->version = 3 - ((input->info->header >> 19) & 0x03);
  if (input->info->version == 3)
    input->info->version = 2;
  input->info->layer = 3 - ((input->info->header >> 17) & 0x03);
  input->info->frequency = ((input->info->header >> 10) & 0x03);

  input->info->size_table_normal = sizes[input->info->version][input->info->layer][input->info->frequency][0];
  input->info->size_table_padded = sizes[input->info->version][input->info->layer][input->info->frequency][1];

  if (input->info->layer == 2) {
    if (!input->info->version) {// mpeg version 1
      if (((input->info->header >> 6) & 0x03) == 3) {
	input->info->info_size = 17;
	input->info->info_read_backref = sideinfo_read_backref_v1_mono;
	input->info->info_read_datasize = sideinfo_read_datasize_v1_mono;
      } else {
	input->info->info_size = 32;
	input->info->info_read_backref = sideinfo_read_backref_v1_stereo;
	input->info->info_read_datasize = sideinfo_read_datasize_v1_stereo;	
      }
    } else {
      if (((input->info->header >> 6) & 0x03) == 3) {
	input->info->info_size = 9;
	input->info->info_read_backref = sideinfo_read_backref_v2_mono;
	input->info->info_read_datasize = sideinfo_read_datasize_v2_mono;
      }	else {
	input->info->info_size = 17;
	input->info->info_read_backref = sideinfo_read_backref_v2_stereo;
	input->info->info_read_datasize = sideinfo_read_datasize_v2_stereo;
      }
    }
  }
}

/*
 *
 */
static unsigned int
header_distance (unsigned int position)
{
  unsigned char c = input->buffer->buffer[position + 2];
  
  if (c & 0x02)
    return input->info->size_table_padded[c >> 4];
  else
    return input->info->size_table_normal[c >> 4];
}

/*
 *
 */
int
input_frame_find_first (void)
{
  unsigned int pos, i, tests;

  for (pos = 0; pos + 3 < input->buffer->size; pos++) {

    if (header_valid (BP(pos)))
      continue;

    input->info->header = (input->buffer->buffer[pos] << 24) | 
      (input->buffer->buffer[pos + 1] << 16) | 
      (input->buffer->buffer[pos + 2] << 8) | 
      (input->buffer->buffer[pos + 3]);
    input->info->header &= 0xffff0dcf;
    
    header_parse ();
    i = pos;
    
    for (tests = 0; tests < DEFAULT_INPUT_HEADER_TEST; tests++) {
      i += header_distance (BP(i));
      log_again (LOG_QUIET, "Testing header %d[%d] for %8x[%d]", tests, i, input->info->header, pos);
      if (header_compatible (BP(i)))
	break;
    }
    if (tests == DEFAULT_INPUT_HEADER_TEST) {
      input->info->header_position = pos;
      log_once (LOG_QUIET, "Valid mpeg stream found: %8x[%d]\n", input->info->header, pos);
      return 0;
    }
  }
  log_once (LOG_QUIET, "Error: This is not a valid mpeg audio stream.");
  return -1;
}

/*
 *
 */
static struct chunk_t *
input_chunk_add (unsigned int type, unsigned int count, unsigned int subcount, 
		 unsigned int position, unsigned int size)
{
  struct chunk_t *chunk = nmalloc (sizeof (struct chunk_t));
  chunk->type = type;
  chunk->count = count;
  chunk->subcount = subcount;
  chunk->position = position;
  chunk->size = size;

  input->chunk_count++;
  input->chunks = nrealloc (input->chunks, input->chunk_count * sizeof (struct chunk_t *));
  input->chunks[input->chunk_count - 1] = chunk;
  return chunk;
}

/*
 *
 */
static struct chunk_t *
input_chunk_insert (unsigned int i, unsigned int type, unsigned int count, unsigned int subcount, 
		 unsigned int position, unsigned int size)
{
  int j;
  struct chunk_t *chunk = nmalloc (sizeof (struct chunk_t));
  chunk->type = type;
  chunk->count = count;
  chunk->subcount = subcount;
  chunk->position = position;
  chunk->size = size;

  input->chunk_count++;
  input->chunks = nrealloc (input->chunks, input->chunk_count * sizeof (struct chunk_t *));

  for (j = chunk_count - 1; j > i; j++)
    input->chunks[j] = input->chunks[j - 1];
    
  input->chunks[j - 1] = chunk;
  return chunk;
}

/*
 *
 */
static void
buffer_print (unsigned int position, unsigned int size)
{
  int i = 0;
  
  if (size > input->buffer->size) {
    log_once (LOG_QUIET, "Not that many data in the buffer");
    exit (0);
  }

  
  for (i = 0; i + 7 < size; i += 7)
    log_once (LOG_QUIET, "%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x", input->buffer->buffer[position + i], 
	      input->buffer->buffer[position + i + 1], input->buffer->buffer[position + i + 2], 
	      input->buffer->buffer[position + i + 3], input->buffer->buffer[position + i + 4],
	      input->buffer->buffer[position + i + 5], input->buffer->buffer[position + i + 6],
	      input->buffer->buffer[position + i + 7]);
}

/*
 *
 */
static void
input_chunk_print (void)
{
  int i;
  struct chunk_t *chunk;
  for (i = 0; i < input->chunk_count; i++) {
    chunk = input->chunks[i];
    switch (chunk->type) {
    case CHUNK_EMPTY:
      log_once (LOG_QUIET, "chunk[%8d]: %10d[%4d]: EMPTY", i, chunk->position, chunk->size);
      break;
    case CHUNK_HEAD:
      log_once (LOG_QUIET, "chunk[%8d]: %10d[%4d]: %05d: HEAD", i, chunk->position, chunk->size, chunk->count);
      break;
    case CHUNK_DATA:
      log_once (LOG_QUIET, "chunk[%8d]: %10d[%4d]: %05d: DATA #%d", i, chunk->position, chunk->size, chunk->count, chunk->subcount);
      break;
    default:
      log_once (LOG_QUIET, "chunk[%8d]: %10d[%4d]: not implemented", i, chunk->position, chunk->size);
      break;
    }
  }
}

/*
 *
 */
static int 
input_segment_layer_3 (int empty) 
{
  struct chunk_t *head_chunk;
  unsigned int backref, datasize;
  unsigned int head_size = 4 + input->info->info_size;
  unsigned int distance;
  unsigned int i;
  unsigned int data_left;

  //buffer_print (0, 100);
  //log_once (LOG_QUIET, "");

  if (!input->chunk_count) {
    if (input->info->header_position) 
      input_chunk_add (CHUNK_EMPTY, 0, 0, 0, input->info->header_position);
    input_chunk_add (CHUNK_HEAD, 0, 0, input->info->header_position, head_size);
  }

  head_chunk = input->chunks[input->chunk_count - 1];
  i = head_chunk->count;
  buffer_print (head_chunk->position, 8);
  log_once (LOG_QUIET, "");
  
  while (i < 25) {
    //log_once (LOG_QUIET, "");
    //buffer_print (head_chunk->position + 4, 32);
    backref = input->info->info_read_backref (BP(head_chunk->position));
    datasize = input->info->info_read_datasize (BP(head_chunk->position));
    distance = header_distance (BP(head_chunk->position));



    log_once (LOG_QUIET, "header %6d position: %8d; backref: %3d; datasize  %4d: distance: %4d", head_chunk->count,
	      head_chunk->position, backref, datasize, distance);

    //log_once (LOG_QUIET, "next header:");
    //buffer_print (head_chunk->position + distance, 8);
    //log_once (LOG_QUIET, "");

    if (backref) {
      unsigned int back_position = head_chunk->position - backref;
      int j = input->chunk_count - 1;
      
      // find out where the data begins
      while (j >= 0) {
	if (input->chunks[j]->type == CHUNK_HEAD)
	  back_position -= head_size;
	else if ((input->chunks[j]->type != CHUNK_EMPTY) || (input->chunks[j]->position <= back_position))
	  break;
	j--;
      }
      if ((j == -1) || (input->chunks[j]->type != CHUNK_EMPTY)) {
	log_once (LOG_QUIET, "Error: Unable to backref: chunk already designated: no handler in place yet.");
	return -1;
      }
      
      // is data neatly alligned with the start of this chunk?
      if (input->chunks[j]->position == back_position) {
	

      } else {

      }
      // end of chunk?
    }
    

    if (header_compatible(BP(head_chunk->position + distance))) {
      log_once (LOG_QUIET, "ERROR: header at %d is not compatible", head_chunk->position + distance);
      exit (0);
    }
    i++;
    
    input_chunk_add (CHUNK_EMPTY, i, 0, head_chunk->position + head_size, distance - head_size);
    head_chunk = input_chunk_add (CHUNK_HEAD, i, 0, head_chunk->position + distance, head_size);
  }
  
  log_once (LOG_QUIET, "");
  input_chunk_print ();
  return 0; 
}

/*
 *
 */
static int 
input_segment_layer_other (int empty)
{
  return 0;
}

/*
 *
 */
int
input_segment (int empty)
{
  if (input->info->layer == 2)
    return input_segment_layer_3 (empty);
  else
    return input_segment_layer_other (empty);
}

/* EOF */
