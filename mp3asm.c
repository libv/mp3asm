/*
 *   mp32asm.c
 *
 *   Oliver Fromme  <olli@fromme.com>
 *   Sun Oct  5 03:19:41 CEST 1997
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>

#include "getlopt.h"
#include "utils.h"
#include "localtypes.h"
#include "frameinfo.h"

static char *outfile = NULL;
static uint32 badbytes = 4096;
static int skip = 0;
static int number = -1;
static int verbose = 0;
static int mp3_fd = -1;	/* mp3 input (file descriptor) */

typedef struct tframe {
	struct tframe *prev;
	struct tframe *next;
	uint32 head;		/* frame header */
	uint8 *info;		/* side info, max. 34 bytes */
	uint8 *data;		/* frame data */
	int isize;		/* size of side info */
	int dsize;		/* size of frame data (deinterleaved) */
	int newfsize;
	int newdsize;
	int backref;
} tframe;

static tframe *firstframe = NULL;
static tframe *lastframe  = NULL;
static tframe *emptyframe = NULL;

static uint8 databufqueue[5120];
static uint8 *databuf;

void *
check_pointer (void *ptr)
{
	if (!ptr) {
		fprintf (stderr, "%s: out of memory.\n", me);
		exit (EX_OSERR);
	}
	return (ptr);
}

#define HSETSIZE 10
static uint32 headerset[HSETSIZE];
static unsigned headercount[HSETSIZE];
static int hsetused = 0;

void
add_headerset (uint32 head)
{
	int i, min, minindex;

	for (i = 0; i < hsetused; i++)
		if (same_stream(head, headerset[i])) {
			headercount[i]++;
			return;
		}
	if (hsetused < HSETSIZE) {
		headerset[hsetused] = head;
		headercount[hsetused] = 1;
		hsetused++;
		return;
	}
	min = headercount[minindex = 0];
	for (i = 0; i < hsetused; i++)
		if (headercount[i] < min)
			min = headercount[minindex = i];
	headerset[minindex] = head;
	headercount[minindex] = 1;
}

void
free_frame (tframe *f)
{
	if (f->info)
		free (f->info);
	if (f->data)
		free (f->data);
	free (f);
}

static tframe *
dup_frame (tframe *f)
{
	tframe *newf;

	check_pointer (newf = malloc(sizeof(*newf)));
	memcpy (newf, f, sizeof(*newf));
	if (f->isize && f->info) {
		check_pointer (newf->info = malloc(f->isize));
		memcpy (newf->info, f->info, f->isize);
	}
	else {
		newf->isize = 0;
		newf->info = NULL;
	}
	if (f->dsize && f->data) {
		check_pointer (newf->data = malloc(f->dsize));
		memcpy (newf->data, f->data, f->dsize);
	}
	else {
		newf->dsize = 0;
		newf->data = NULL;
	}
	return (newf);
}

void
analyse_headerset (void)
{
	tframe *f, *dumpf;
	int dumped, saved;
	int i, max, maxindex;
	uint32 head;

	if (hsetused < 2)
		return;
	max = headercount[maxindex = 0];
	for (i = 0; i < hsetused; i++)
		if (headercount[i] > max)
			max = headercount[maxindex = i];
	head = headerset[maxindex];
	f = firstframe;
	dumped = 0;
	saved = 0;
	while (f) {
		if (!same_stream(f->head, head)) {
			dumpf = f;
			f = f->next;
			if (f)
				f->prev = dumpf->prev;
			else
				lastframe = dumpf->prev;
			if (dumpf->prev)
				dumpf->prev->next = f;
			else
				firstframe = f;
			free_frame (dumpf);
			dumped++;
		}
		else {
			if (!f->dsize && !emptyframe) {
				/*
				 *   We might need a prototype of an empty
				 *   frame later, so we look for one and
				 *   duplicate it.  This is a dirty hack,
				 *   but it works most of the time...
				 */
				emptyframe = dup_frame(f);
			}
			f = f->next;
			saved++;
		}
	}
	if (dumped && verbose >= 1) {
		fprintf (stderr, "%s: dropping %d probably broken frames.\n",
		    me, dumped);
		fprintf (stderr, "%s: %d frames left.\n", me, saved);
	}
}

int
wread (int fd, uint8 *buf, int count)
{
	int readb;

	while (count) {
		if ((readb = read(fd, buf, count)) < 0)
			if (errno == EINTR)
				continue;
			else {
				fprintf (stderr, "%s: read error\n", me);
				perror ("read()");
				return (0);
			}
		if (!readb) /* EOF */
			return (2);
		count -= readb;
		buf += readb;
	}
	return (1);
}

uint32
read_head (uint32 maxtries)
{
	uint8 bbuf[4];
	uint32 head;
	uint32 try;

	switch (wread(mp3_fd, bbuf, 4)) {
	case 1:
		break;
	case 2:
		return (2); /* EOF */
	default:
		fprintf (stderr,
		    "%s: error reading frame header.\n", me);
		return (0);
	}
	head = (uint32) bbuf[0] << 24
		| (uint32) bbuf[1] << 16
		| (uint32) bbuf[2] << 8
		| (uint32) bbuf[3];
	try = 0;
	while (!valid_framehead(head)) {
		try++;
		if (maxtries--) {
			switch (wread(mp3_fd, bbuf, 1)) {
			case 1:
				break;
			case 2:
				if (verbose >= 1)
					fprintf (stderr, "%s: skipped %d "
					    "bad bytes.\n", me, try + 3);
				return (2); /* EOF */
			default:
				fprintf (stderr,
				    "%s: error reading frame header.\n",
				    me);
				return (0);
			}
			head = (head & 0xffffff) << 8 | bbuf[0];
		}
		else {
			fprintf (stderr, "%s: can't find "
			    "valid MPEG audio header\n", me);
			errno = 0;
			return (0);
		}
	}
	if (try)
		fprintf (stderr, "%s: skipped %d bad bytes.\n", me, try);
	return (head);
}

static int framenum = 0;
static int lastignore = 1;
static int lastbackref = 0;
static int lastlocdsize = 0;
static tframe *lastf = NULL;

static void
reset_read_frame (void)
{
	framenum = 0;
	lastignore = 1;
	lastbackref = 0;
	lastlocdsize = 0;
	lastf = NULL;
}

static int
transfer_frame_data (void)
{
	uint8 *data;
	int i, empty;

	data = databuf - (lastbackref + lastlocdsize);
	empty = 1;
	for (i = 0; i < lastf->dsize; i++)
		if (data[i] != 0xff) {
			empty = 0;
			break;
		}
	if (!empty) {
		check_pointer (lastf->data = (uint8 *) malloc(lastf->dsize));
		memcpy (lastf->data, data, lastf->dsize);
	}
	else {
		lastf->dsize = 0;
		lastf->data = NULL;
	}
	return (1);
}

static int
read_frame (tframe *f, int ignore)
{
	int backref;
	int locdsize;
	uint8 info[34];

	if (verbose >= 3)
		if (verbose >= 4 || !(framenum & 0x007f))
			fprintf (stderr, "\r{%d} ", framenum);
	do
		switch ((f->head = read_head(badbytes))) {
		case 0:
			return (0); /* error */
		case 2:
			return (2); /* EOF */
		}
	while
	    (!get_framedata(f->head, &f->isize, &locdsize) || locdsize <= 0);
	if (!ignore)
		add_headerset (f->head);
	if (f->isize > 0) {
		if (ignore)
			f->info = info;
		else
			check_pointer (f->info = (uint8 *) malloc(f->isize));
		switch (wread(mp3_fd, f->info, f->isize)) {
		case 1:
			break;
		case 2:
			return (2); /* EOF */
		default:
			fprintf (stderr,
			    "%s: error reading frame side info.\n", me);
			return (0);
		}
		backref = ((unsigned) f->info[0] << 1)
			| ((unsigned) f->info[1] >> 7);
	}
	else {
		f->isize = 0;
		f->info = NULL;
		backref = 0;
	}
	switch (wread(mp3_fd, databuf, locdsize)) {
	case 1:
		break;
	case 2:
		return (2); /* EOF */
	default:
		fprintf (stderr, "%s: error reading frame data.\n", me);
		return (0);
	}
	if (lastf) {
		lastf->dsize = (lastlocdsize + lastbackref) - backref;
		if (lastf->dsize < 0)
			lastf->dsize = 0;
		if (!lastignore)
			if (!transfer_frame_data())
				return (0);
	}
	if (locdsize)
		memmove (databufqueue, databufqueue + locdsize, 2560);
	lastignore = ignore;
	lastlocdsize = locdsize;
	lastbackref = backref;
	lastf = f;
	framenum++;
	return (1);
}

static int
read_last_frame (void)
{
	if (lastf) {
		lastf->dsize = lastlocdsize + lastbackref;
		if (!lastignore)
			if (!transfer_frame_data())
				return (0);
	}
	return (1);
}

static void
read_stream (char *name, int skip, int count)
{
	tframe *f, dummyf;
	int result, start, end;

	if ((mp3_fd = open(name, O_RDONLY, 0)) < 0) {
		fprintf (stderr, "%s: ", me);
		perror (name);
		exit (EX_NOINPUT);
	}
	if (verbose >= 2)
		fprintf (stderr, "Reading from %s ...\n", name);
	result = 1;
	while (result != 2 && skip--) {
		if ((result = read_frame(&dummyf, 1)) == 2)
			read_last_frame ();
		else if (result != 1) {
			fprintf (stderr, "%s: error reading frame.\n", name);
			exit (EX_UNAVAILABLE);
		}
	}
	start = framenum;
	while (result != 2 && count--) {
		check_pointer (f = malloc(sizeof(*f)));
		memset (f, 0, sizeof(*f));
		if ((result = read_frame(f, 0)) == 2) {
			read_last_frame ();
			free (f);
			break;
		}
		else if (result != 1) {
			fprintf (stderr, "%s: error reading frame.\n", name);
			exit (EX_UNAVAILABLE);
		}
		if (!firstframe) {
			firstframe = lastframe = f;
			f->prev = NULL;
		}
		else {
			f->prev = lastframe;
			lastframe->next = f;
			lastframe = f;
		}
	}
	end = framenum;
	reset_read_frame();
	close (mp3_fd);
	if (lastframe)
		lastframe->next = NULL;
	if (verbose >= 1)
		if (end - start)
			fprintf (stderr,
			    "\r%s:\t%d - %d %s(%d)\n", name,
			    start, end - 1, result == 2 ? "[EOF] " : "",
			    end - start);
		else
			fprintf (stderr,
			    "\r%s:\tno frames within given range\n", name);
}

static void
grow_frame (tframe *f, int num)
{
	if (verbose >= 4)
		fprintf (stderr, "Note: frame %d data underflow, "
		    "inserting %d pad bytes.\n", framenum - 1, num);
	if (f->data)
		f->data = realloc(f->data, f->dsize + num);
	else
		f->data = malloc(f->dsize + num);
	check_pointer (f->data);
	memset (f->data + f->dsize, 0xff, num);
	f->dsize += num;
}

tframe *
insert_emptyframe (tframe *f)
{
	tframe *insf;

	if (verbose >= 1)
		fprintf (stderr, "Warning: frame %d data overflow, "
		    "have to insert empty frame.\n", framenum);
	if (!emptyframe) {
		fprintf (stderr, "%s: internal error (too "
		    "many dirty hacks)\n", me);
		exit (EX_SOFTWARE);
	}
	insf = dup_frame(emptyframe);
	insf->next = f;
	if ((insf->prev = f->prev))
		f->prev->next = insf;
	else
		firstframe = insf;
	return (f->prev = insf);
}

void
interleave_stream (void)
{
	int32 bpf_mul, bpf_div;
	tframe *f;
	frameinfo fi;
	int fsize, normsize;
	int backref, oldbackref;
	int bres, oldbres;
	int ddiff;

	
	if (verbose >= 2)
		fprintf (stderr, "Building new frame structure ...\n");
	if (!get_frameinfo(firstframe->head, &fi)) {
		fprintf (stderr, "%s: not a valid MPEG audio stream.\n", me);
		exit (EX_UNAVAILABLE);
	}
	framenum = 0;
	bpf_mul = fi.kbps * 125 * fi.spf;
	bpf_div = fi.freq;
	normsize = bpf_mul / bpf_div;
	backref = 0;
	bres = 0;
	if (!(f = firstframe))
		return;
	for (;;) {
		/*
		 *   First calculate the frame size, based
		 *   on the target bit rate.  Note that this
		 *   has to be as accurate as possible!
		 *   We're using the Bresenham algorithm, so
		 *   the average deviation is always smaller
		 *   than one byte, according to the target
		 *   bit rate.
		 *
		 *   This approach doesn't work with layer-1
		 *   streams, but who uses layer-1 anyway...
		 */
		oldbres = bres;
		for (fsize = 1;; fsize++)
			if ((bres += bpf_div) >= bpf_mul) {
				bres -= bpf_mul;
				break;
			}
		f->newfsize = fsize;
		f->newdsize = fsize - (f->isize + 4);
		if (fsize == normsize)
			f->head = f->head & ~((uint32) 0x0200);
		else
			f->head = f->head | 0x0200;
		/*
		 *   Now calculate the back reference pointer,
		 *   which indicates the starting offset of the
		 *   actual frame data.  We have 9 bits for
		 *   this value, so it must be within the range
		 *   from 0 to 511.
		 */
		if (backref > 511) {
			/*
			 *   If the back reference exceeds 511,
			 *   we have to insert pad bytes in the
			 *   previous frame.
			 */
			grow_frame (f->prev, backref - 511);
			backref = 511;
		}
		f->info[0] = (backref >> 1) & 0xff;
		f->info[1] = ((backref << 7) & 0x80) | (f->info[1] & 0x7f);
		oldbackref = f->backref = backref;
		backref += f->newdsize - f->dsize;
		if (backref < 0) {
			/*
			 *   We're in trouble now.
			 *   The data belonging to the frame
			 *   was too large.  We simply insert
			 *   an empty frame before this one.
			 *   This is not the optimal solution
			 *   (in fact it's a dirty hack), but
			 *   at least we stay compatible with
			 *   the MPEG audio specifications.
			 */
			backref = oldbackref;
			bres = oldbres;
			f = insert_emptyframe (f);
			/*
			 *   Now continue interleaving with
			 *   the empty frame that has just
			 *   been inserted.
			 */
			continue;
		}
		framenum++;
		if ((f = f->next))
			continue;
		/*
		 *   Finally, we have to check the
		 *   length of the last frame -- its
		 *   data should end with the end of
		 *   the frame.
		 *   If necessary, we have to add pad
		 *   bytes (to make it longer), or to
		 *   insert another empty frame (to
		 *   make room for the last frame's
		 *   data).
		 */
		f = lastframe;
		ddiff = (f->backref + f->newdsize) - f->dsize;
		if (ddiff > 0)
			grow_frame (f, ddiff);
		else if (ddiff < 0) {
			backref = oldbackref;
			bres = oldbres;
			f = insert_emptyframe (f);
			continue;
		}
		break; /* Done. */
	}
}

static void
write_stream (void)
{
	tframe *f, *df;
	int usestdout;
	int doffset, dcount, dleft;
	uint8 hbuf[4];

	usestdout = (strcmp(outfile, "-") == 0);
	if (usestdout)
		mp3_fd = STDOUT_FILENO;
	else
		if ((mp3_fd =
		    open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
			fprintf (stderr, "%s: ", me);
			perror (usestdout ? "stdout" : outfile);
			exit (EX_IOERR);
		}
	if (verbose >= 2)
		fprintf (stderr, "Writing to %s ...\n",
		   usestdout ? "stdout" : outfile);
	doffset = 0;
	framenum = 0;
	for (f = df = firstframe; f; f = f->next) {
		if (verbose >= 3)
			if (verbose >= 4 || !(framenum & 0x007f))
				fprintf (stderr, "\r{%d} ", framenum);
		hbuf[0] = (f->head >> 24) & 0xff;
		hbuf[1] = (f->head >> 16) & 0xff;
		hbuf[2] = (f->head >> 8) & 0xff;
		hbuf[3] = f->head & 0xff;
		write (mp3_fd, hbuf, 4);
		if (f->info && f->isize)
			write (mp3_fd, f->info, f->isize);
		dcount = f->newdsize;
		while (df && dcount) {
			if ((dleft = df->dsize - doffset) > dcount)
				dleft = dcount;
			write (mp3_fd, df->data + doffset, dleft);
			dcount -= dleft;
			doffset += dleft;
			while (df && doffset == df->dsize) {
				doffset = 0;
				df = df->next;
			}
		}
		framenum++;
	}
	if (!usestdout)
		close (mp3_fd);
	if (verbose >= 1)
		fprintf (stderr, "\r%s: Wrote %d frames.\n", me, framenum);
}

void
inc_verb (char *arg)
{
	verbose++;
}

void
inc_badb (char *arg)
{
	if (badbytes < 40000000)
		badbytes *= 10;
}

void
usage (char *arg)
{
	fprintf (stderr, "Usage: %s [options] file.mp3 [...]\n", me);
	fprintf (stderr, "Options and files can appear in any order.  Options affect all files\n");
	fprintf (stderr, "specified AFTER them, so the order is important.  Known options:\n");
	fprintf (stderr, "   -v     increase verbosity level (default = silent)\n");
	fprintf (stderr, "   -y     try harder to read seriously broken files\n");
	fprintf (stderr, "   -s n   skip first n frames in input files (default = 0)\n");
	fprintf (stderr, "   -n n   only read n frames from input files (default = -1 = all)\n");
	fprintf (stderr, "   -o f   output to file f, - = stdout (default = none)\n");
	exit (EX_USAGE);
}

const topt myopts[] = {
	{'v', 0, GLO_NONE, inc_verb, 0,        0},
	{'y', 0, GLO_NONE, inc_badb, 0,        0},
	{'s', 0, GLO_NUM,  0,        &skip,    0},
	{'n', 0, GLO_NUM,  0,        &number,  0},
	{'o', 0, GLO_CHAR, 0,        &outfile, 0},
	{'h', "help", GLO_NONE, usage, 0, 0},
	{'?', 0, GLO_NONE, usage, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

void
clean_up (void)
{
	tframe *f;

	while ((f = firstframe)) {
		firstframe = f->next;
		free_frame (f);
	}
	if (emptyframe)
		free_frame (emptyframe);
}

int
main (int argc, char *argv[])
{
	utils_init (argv[0]);
	memset (databufqueue, 0xff, 5120);
	databuf = databufqueue + 2560;
	if (argc < 2)
		usage (NULL);
	while (loptind < argc) {
		parselopts (argc, argv, myopts, me);
		if (loptind < argc)
			read_stream (argv[loptind], skip, number);
		loptind++;
	}
	if (firstframe) {
		analyse_headerset();
		if (outfile) {
			interleave_stream();
			write_stream();
		}
		clean_up();
	}
	else
		fprintf (stderr, "%s: nothing to do (no frames).\n", me);
	if (outfile)
		free (outfile);
	exit (EX_OK);
}

/* EOF */    
