/** @file
    Time Zone processing, declarations and macros.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Derived from the NIH time zone package file, tzfile.h, which contains the following notice:

    This file is in the public domain, so clarified as of
    1996-06-05 by Arthur David Olson (arthur_david_olson@nih.gov).

    This header is for use ONLY with the time conversion code.
    There is no guarantee that it will remain unchanged,
    or that it will remain at all.
    Do NOT copy it to any system include directory.
    Thank you!

    NetBSD: tzfile.h,v 1.8 1998/01/22 07:06:59 jtc Exp
**/
#ifndef TZFILE_H
#define TZFILE_H

/*
** Information about time zone files.
*/

#ifndef TZDIR   /* Time zone object file directory */
#define TZDIR   "/usr/share/zoneinfo"
#endif /* !defined TZDIR */

#ifndef TZDEFAULT
#define TZDEFAULT "/etc/localtime"
#endif /* !defined TZDEFAULT */

#ifndef TZDEFRULES
#define TZDEFRULES  "posixrules"
#endif /* !defined TZDEFRULES */

/*
** Each file begins with. . .
*/

#define TZ_MAGIC  "TZif"

struct tzhead {
  char  tzh_magic[4];   /* TZ_MAGIC */
  char  tzh_reserved[16]; /* reserved for future use */
  char  tzh_ttisgmtcnt[4];  /* coded number of trans. time flags */
  char  tzh_ttisstdcnt[4];  /* coded number of trans. time flags */
  char  tzh_leapcnt[4];   /* coded number of leap seconds */
  char  tzh_timecnt[4];   /* coded number of transition times */
  char  tzh_typecnt[4];   /* coded number of local time types */
  char  tzh_charcnt[4];   /* coded number of abbr. chars */
};

/*
** . . .followed by. . .
**
**  tzh_timecnt (char [4])s   coded transition times a la time(2)
**  tzh_timecnt (unsigned char)s  types of local time starting at above
**  tzh_typecnt repetitions of
**    one (char [4])    coded UTC offset in seconds
**    one (unsigned char) used to set tm_isdst
**    one (unsigned char) that's an abbreviation list index
**  tzh_charcnt (char)s   '\0'-terminated zone abbreviations
**  tzh_leapcnt repetitions of
**    one (char [4])    coded leap second transition times
**    one (char [4])    total correction after above
**  tzh_ttisstdcnt (char)s    indexed by type; if TRUE, transition
**          time is standard time, if FALSE,
**          transition time is wall clock time
**          if absent, transition times are
**          assumed to be wall clock time
**  tzh_ttisgmtcnt (char)s    indexed by type; if TRUE, transition
**          time is UTC, if FALSE,
**          transition time is local time
**          if absent, transition times are
**          assumed to be local time
*/

/*
** In the current implementation, "tzset()" refuses to deal with files that
** exceed any of the limits below.
*/

#ifndef TZ_MAX_TIMES
/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES  370
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
#ifndef NOSOLAR
#define TZ_MAX_TYPES  256 /* Limited by what (unsigned char)'s can hold */
#endif /* !defined NOSOLAR */
#ifdef NOSOLAR
/*
** Must be at least 14 for Europe/Riga as of Jan 12 1995,
** as noted by Earl Chew <earl@hpato.aus.hp.com>.
*/
#define TZ_MAX_TYPES  20  /* Maximum number of local time types */
#endif /* !defined NOSOLAR */
#endif /* !defined TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS  50  /* Maximum number of abbreviation characters */
        /* (limited by what unsigned chars can hold) */
#endif /* !defined TZ_MAX_CHARS */

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS  50  /* Maximum number of leap second corrections */
#endif /* !defined TZ_MAX_LEAPS */

#define SECSPERMIN  60
#define MINSPERHOUR 60
#define HOURSPERDAY 24
#define DAYSPERWEEK 7
#define DAYSPERNYEAR  365
#define DAYSPERLYEAR  366
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY  ((LONG32)(SECSPERHOUR * HOURSPERDAY))
#define MONSPERYEAR 12

#define EPOCH_YEAR  1970
#define EPOCH_WDAY  TM_THURSDAY   // Use this for 32-bit time_t
//#define EPOCH_WDAY  TM_SUNDAY     // Use this for 64-bit time_t

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#endif /* !defined TZFILE_H */
