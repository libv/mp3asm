/*
 *   frameinfo.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Sat Jul 26 01:05:42 MET DST 1997
 */

#include "localtypes.h"
#include "frameinfo.h"

/*
 *   This constant array is optimized for fast access by the
 *   get_framedata() function which is called by the mp32var
 *   module for every single frame.  It is not quite optimized
 *   for access by the get_frameinfo() function, but that is
 *   usually only called once per MPEG stream.
 *
 *   1st index:  0 = "MPEG 1.0",   1 = "MPEG 2.0"
 *   2nd index:  0 unused,   1 = "Layer 3",   2 = "Layer 2",   3 = "Layer 1"
 *   3rd index:  bitrate index from frame header
 */
static const int kbpstab[2][4][16] =
{{
	{0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
	{0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320},
	{0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384},
	{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}
},
{
	{0,  0,  0,  0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0},
	{0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160},
	{0,  8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160},
	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}
}};

static const unsigned freqtab[7] =
	{44100, 48000, 32000, 22050, 24000, 16000, 11025};

int get_frameinfo (uint32 head, frameinfo *finfo)
{
	int bridx; /* bitrate index */

	if ((head & 0xffe00000) != 0xffe00000) {
		/*
		 *   Problem.
		 *   Currently we don't handle this in a very
		 *   clever way...
		 */
		/*FIXME*/
		return (0);
	}
	if ((head & 0x0c00) == 0x0c00) {
		return (0);
	}
	/*LINTED*/
	if (!(bridx = 0x0f & head >> 12)) {
		return (0);
	}
	if ((finfo->layer = 4 - (0x03 & head >> 17)) > 3) {
		return (0);
	}
	if (head & 0x00100000) {
		if (head & 0x00080000) { /* MPEG 1.0 */
			finfo->ver_maj = 1;
			finfo->freq = freqtab[0x03 & head >> 10];
		}
		else { /* MPEG 2.0 */
			finfo->ver_maj = 2;
			finfo->freq = freqtab[3 + (0x03 & head >> 10)];
		}
		finfo->ver_min = 0;
	}
	else { /* "MPEG 2.5" */
		finfo->freq = freqtab[6];
		finfo->ver_maj = 2;
		finfo->ver_min = 5;
	}
	finfo->kbps = kbpstab[finfo->ver_maj - 1][4 - finfo->layer][bridx];

	finfo->flags = 0;
	/*FIXME*/ /*XXX*/
	/* Ein paar von diesen Flags sind garantiert negiert! */
	if (!(head & 0x10000))
		finfo->flags |= FF_ERRORPROTECTION;
	if (head & 0x0200)
		finfo->flags |= FF_FRAMEPADDING;
	if (!(head & 0x0100))
		finfo->flags |= FF_EXTENSION;
	if (!(head & 0x08))
		finfo->flags |= FF_COPYRIGHT;
	if (!(head & 0x04))
		finfo->flags |= FF_ORIGINAL;

	if ((finfo->mode = 0x03 & head >> 6) == MODE_MONO)
		finfo->channels = 1;
	else
		finfo->channels = 2;
	finfo->mode_ext = 0x03 & head >> 4;
	finfo->emphasis = 0x02 & head;

	if (finfo->layer == 3)
		finfo->spf = (finfo->ver_maj == 1) ? 1152 : 576;
	else if (finfo->layer == 2)
		finfo->spf = 1152;
	else
		finfo->spf = 384;
	return (1);
}

#define STREAM_EQMASK		0xfffffcc0

int same_stream (uint32 head1, uint32 head2)
{
	return (((head1 ^ head2) & STREAM_EQMASK) == 0);
}

#define FRAMEHEAD_EQMASK	0xfffffec0

int get_framedata (uint32 head, int *isize, int *dsize)
{
	int freq;	/* sample rate */
	int laymi;	/* 4 - audio layer */
	int kbps;	/* bit rate */
	int l3spf;	/* samples per frame for Layer 3 */
	int bpf;	/* bytes per frame */
	int bpsi;	/* bytes per side info */

	static uint32 last_head  = 0xfffb9044 & FRAMEHEAD_EQMASK;
	static int    last_isize = 32;
	static int    last_dsize = 381;

	if ((head & FRAMEHEAD_EQMASK) == last_head) {
		*isize = last_isize;
		*dsize = last_dsize;
		return (1);
	}
	if ((head & 0xffe00000) != 0xffe00000) {
		/*
		 *   Problem.
		 *   Currently we don't handle this in a very
		 *   clever way...
		 */
		/*FIXME*/
		return (0);
	}
	if ((head & 0x0c00) == 0x0c00) {
		return (0);
	}
	switch (head & 0x00180000) {
	case 0x00180000:	/* MPEG 1.0 */
		freq = freqtab[0x03 & head >> 10];
		kbps = kbpstab[0][laymi = 0x03 & head >> 17][0x0f & head >> 12];
		l3spf = 1152;
		bpsi = ((head & 0x00c0) == 0x00c0) ? 17 : 32;
		break;
	case 0x00100000:	/* MPEG 2.0 */
		freq = freqtab[3 + (0x03 & head >> 10)];
		kbps = kbpstab[1][laymi = 0x03 & head >> 17][0x0f & head >> 12];
		l3spf = 576;
		bpsi = ((head & 0x00c0) == 0x00c0) ? 9 : 17;
		break;
	default:		/* "MPEG 2.5" */
		freq = freqtab[6];
		kbps = kbpstab[1][laymi = 0x03 & head >> 17][0x0f & head >> 12];
		l3spf = 576;
		bpsi = ((head & 0x00c0) == 0x00c0) ? 9 : 17;
	}
	switch (laymi) {
	case 1:	/* Layer 3 */
		bpf = (kbps * 125 * l3spf) / freq;
		if (head & 0x0200)
			bpf++;
		if (!(head & 0x10000))
			bpsi += 2;
		break;
	case 2: /* Layer 2 */
		bpf = (kbps * 125 * 1152) / freq;
		if (head & 0x0200)
			bpf++;
		bpsi = 0;
		break;
	case 3: /* Layer 1 */
		bpf = (kbps * 125 * 384) / (freq << 2) << 2;
		if (head & 0x0200)
			bpf += 4;
		bpsi = 0;
		break;
	default:
		return (0);
	}
	last_head = head & FRAMEHEAD_EQMASK;
	*isize = last_isize = bpsi;
	*dsize = last_dsize = bpf - bpsi - 4;
	return (1);
}

int valid_framehead (uint32 head)
{
	if ((head & 0xffe00000) != 0xffe00000 ||
	    (head & 0x0c00) == 0x0c00 ||
	    !(head & 0x060000)) {
		return (0);
	}
	else
		return (1);
}

/* EOF */
