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
#ifndef HAVE_LOGGING_H
#define HAVE_LOGGING_H

// Implements logging to stderr or file.

#include <stdarg.h>

#define LOG_QUIET 0
#define LOG_BRIEF 1
#define LOG_NORMAL 2
#define LOG_NOISY 3
#define LOG_LOUD 4
#define LOG_DEBUG 5

#define LOG_MIN LOG_QUIET
#define LOG_MAX LOG_LOUD

// address space seperation is a good thing but not really feature of C
void log_once (int level, char *format,  ...);
void log_again (int level, char *format,  ...);

#endif // LOGGING_H
