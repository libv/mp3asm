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

/*
 * new_block:
 *
 */
static void
print_block (block_t *block)
{
  char buf[20];
  switch (block->type)
    {
    case PADDING:
      sprintf (buf, "Padding");
      break;
    case HEADER:
      sprintf (buf, "Header");
      break;
    case SIDEINFO1:
      sprintf (buf, "Sideinfo v1");
      break;
    case SIDEINFO2:
      sprintf (buf, "Sideinfo v2");
      break;
    case DATA:
      sprintf (buf, "Data");
      break;
    case ID3V1:
      sprintf (buf, "ID3 v1");
      break;
    case ID3V2:
      sprintf (buf, "ID3 v2");
      break;
    case BAD:
      sprintf (buf, "BAD");
      break;
    default:
      sprintf (buf, "Unknown");
    }
  fprintf(stderr, "%s block on %ld, %d large\n", buf,  block->pos, block->size);
}

/*
 * new_block:
 *
 */
block_t *
init_block (int filenr, long pos, guint16 size, unsigned char type)
{
  block_t *block = tmalloc (sizeof(block_t));
  
  block->filenr = filenr;
  block->pos = pos;
  block->size = size;
  block->type = type;
  return (block);
  switch (type)
    {
    case HEADER:
      block->data = tmalloc(sizeof(header_t));
    case INFO1:
      block->data = tmalloc(sizeof(info1_t));
    case INFO2:
      block->data = tmalloc(sizeof(info2_t));
    default:
      block->data = NULL;
    }
  print_block (block);
  return (block);
}

/*
 * add_block: adds block at the end of a stream
 *
 */
block_t *
add_block (block_t **list, int *count, int filenr, unsigned long *pos, guint16 size,
	   unsigned char type)
{
  *count++;
  list = trealloc(list, (*count + 1)*sizeof(block_t *));
  *pos += size;
  return (list[count] = new_block (filenr, *pos, size, type));
}

/*
 * split_block: splits a block, data is copied (adjusted too)
 *  U should be put to death when u use this for head/info/id3v1
 */
block_t *
split_block (block_t **list, int *count, int *count2, int *pos)
{
  block_t *block1 = list[count2], block2;
  int i;	  

  count++;
  list = trealloc (list, (count + 1)*sizeof(block_t *));

  for (i = count - 1; i > count2; i--)
    list[i + 1] = list[i];

  block2 = list[count2 + 1] = new_block (block1->filenr, block1->pos + pos,
					 block1->size - pos, block1->type);
  block1->size = pos;

  switch (block1->type)
    {
    case DATA:
    case OVERLAPPING:
    case BAD:
      block2->data = memcpy (block2->data, block1->data, sizeof(data_t));
      block1->data->nextp = block2;
      break;
    case OVERLAPPED:
    case OVERLAPPED_BAD:
      block2->data = memcpy (block2->data, block1->data, sizeof(overlap_t));
      block1->data->nextp = block2;
      break;
    }
  count2++;
  return (block2);
}
data_t *
new_data (block_t *next, block_t *headp)
{
  data_t *data = tmalloc(sizeof(data_t));
  block->data->next = next;
  block->data->headp = headp;
  return (data);
}

