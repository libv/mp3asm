/*
 *   frameinfo.h
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Sat Jul 26 01:02:26 MET DST 1997
 */

#ifndef HAVE_FRAMEINFO_H
#define HAVE_FRAMEINFO_H

/*-----------------------------------------------------------*/

#include <sys/types.h>

/*
 *   This struct contains the frame/stream information that is
 *   transferred from converter to coordinator when an input
 *   file has been successfully opened.
 *   Some conversion formulas:
 *      frames_per_second = freq / spf
 *      seconds_per_file  = fsize / (kbps * 125)
 *      bytes_per_frame   = (kbps * 125 * spf) / freq
 *         (Layer 1 rounds to 4-byte boundaries, the other
 *         layers round to 1-byte boundaries.)
 */
typedef struct {
	unsigned ver_maj;	/* major MPEG version number (1 or 2) */
	unsigned ver_min;	/* minor MPEG version number (0) */
	unsigned layer;		/* audio layer (1, 2 or 3) */
	off_t fsize;		/* file size in bytes (not set!) */
	unsigned kbps;		/* bitrate (kilobits per second, e.g. 128) */
	unsigned freq;		/* samplerate (Hz, e.g. 44100) */
	unsigned channels;	/* 1 = mono, 2 = stereo */
	unsigned spf;		/* samples per frame (1152 or 576) */
	unsigned flags;		/* see below */
	unsigned mode;		/* see below (0 - 3) */
	unsigned mode_ext;	/* mode extension */
	unsigned emphasis;	/* emphasis */
} frameinfo;

/*
 *   frameinfo.flags
 */
#define FF_ERRORPROTECTION	0x01
#define FF_FRAMEPADDING		0x02
#define FF_EXTENSION		0x04
#define FF_COPYRIGHT		0x08
#define FF_ORIGINAL		0x10

/*
 *   frameinfo.mode
 */
#define MODE_STEREO		0x00
#define MODE_JOINT_STEREO	0x01
#define MODE_DUAL_CHANNEL	0x02
#define MODE_MONO		0x03

int get_frameinfo (uint32 head, frameinfo *finfo);
/*
 *   Returns 1 on success, 0 on format error.
 */

/*-----------------------------------------------------------*/

int get_framedata (uint32 head, int *isize, int *dsize);
/*
 *   Result:  1 = ok, 0 = format error.
 *   Returns the following values via pointers:
 *      *isize     size of side info (0 if layer < 3)
 *      *dsize     data size (without side info, if any)
 */

/*-----------------------------------------------------------*/

int same_stream (uint32 head1, uint32 head2);
/*
 *   Result: 1 = heads are "compatible".
 *           0 = heads don't belong to the same stream.
 */

/*-----------------------------------------------------------*/

int valid_framehead (uint32 head);
/*
 *   Result: 1 = valid frame header, 0 = invalid.
 */

/*-----------------------------------------------------------*/

#endif /* HAVE_FRAMEINFO_H */

/* EOF */
