/** @file
    Time Zone processing.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Portions derived from the NIH time zone package file, localtime.c,
  which contains the following notice:

    This file is in the public domain, so clarified as of
    1996-06-05 by Arthur David Olson (arthur_david_olson@nih.gov).

  NetBSD: localtime.c,v 1.39 2006/03/22 14:01:30 christos Exp
**/
#include  <LibConfig.h>

#include  <ctype.h>
#include  <fcntl.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <time.h>
#include  <unistd.h>
#include  "tzfile.h"
#include  "TimeVals.h"

#ifndef WILDABBR
/*
** Someone might make incorrect use of a time zone abbreviation:
**  1.  They might reference tzname[0] before calling tzset (explicitly
**    or implicitly).
**  2.  They might reference tzname[1] before calling tzset (explicitly
**    or implicitly).
**  3.  They might reference tzname[1] after setting to a time zone
**    in which Daylight Saving Time is never observed.
**  4.  They might reference tzname[0] after setting to a time zone
**    in which Standard Time is never observed.
**  5.  They might reference tm.TM_ZONE after calling offtime.
** What's best to do in the above cases is open to debate;
** for now, we just set things up so that in any of the five cases
** WILDABBR is used.  Another possibility:  initialize tzname[0] to the
** string "tzname[0] used before set", and similarly for the other cases.
** And another:  initialize tzname[0] to "ERA", with an explanation in the
** manual page of what this "time zone abbreviation" means (doing this so
** that tzname[0] has the "normal" length of three characters).
*/
#define WILDABBR  "   "
#endif /* !defined WILDABBR */

const char wildabbr[9]  = "WILDABBR";
const char gmt[4]       = "GMT";

struct state * lclptr = NULL;
struct state * gmtptr = NULL;

#ifndef TZ_STRLEN_MAX
#define TZ_STRLEN_MAX 255
#endif /* !defined TZ_STRLEN_MAX */

static char   lcl_TZname[TZ_STRLEN_MAX + 1];
static int    lcl_is_set = 0;
//static int    gmt_is_set = 0;

char *   tzname[2] = {
  (char *)__UNCONST(wildabbr),
  (char *)__UNCONST(wildabbr)
};

long int    timezone = 0;
int         daylight = 0;

#ifndef NO_ZONEINFO_FILES
/** Get first 4 characters of codep as a 32-bit integer.

    The first character of codep becomes the MSB of the resultant integer.
**/
static INT32
detzcode(const char * const codep)
{
  register INT32 result;

  /*
  ** The first character must be sign extended on systems with >32bit
  ** longs.  This was solved differently in the master tzcode sources
  ** (the fix first appeared in tzcode95c.tar.gz).  But I believe
  ** that this implementation is superior.
  */
#define SIGN_EXTEND_CHAR(x) ((signed char) x)

  result = (SIGN_EXTEND_CHAR(codep[0]) << 24) \
    | (codep[1] & 0xff) << 16 \
    | (codep[2] & 0xff) << 8
    | (codep[3] & 0xff);
  return result;
}
#endif  /* NO_ZONEINFO_FILES */

static void
settzname (void)
{
  register struct state * const sp = lclptr;
  register int      i;

  tzname[0] = (char *)__UNCONST(wildabbr);
  tzname[1] = (char *)__UNCONST(wildabbr);
  daylight = 0;
  timezone = 0;
  if (sp == NULL) {
    tzname[0] = tzname[1] = (char *)__UNCONST(gmt);
    return;
  }
  for (i = 0; i < sp->typecnt; ++i) {
    register const struct ttinfo * const  ttisp = &sp->ttis[i];

    tzname[ttisp->tt_isdst] =
      &sp->chars[ttisp->tt_abbrind];
    if (ttisp->tt_isdst)
      daylight = 1;
    if (i == 0 || !ttisp->tt_isdst)
      timezone = -(ttisp->tt_gmtoff);
  }
  /*
  ** And to get the latest zone names into tzname. . .
  */
  for (i = 0; i < sp->timecnt; ++i) {
    register const struct ttinfo * const  ttisp =
      &sp->ttis[ sp->types[i] ];

    tzname[ttisp->tt_isdst] =
      &sp->chars[ttisp->tt_abbrind];
  }
}

/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found.  Return a pointer to that
** character.
*/
static const char *
getzname(register const char *strp)
{
  register char c;

  while ((c = *strp) != '\0' && !is_digit(c) && c != ',' && c != '-' &&
         c != '+')
    ++strp;
  return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/
static const char *
getnum(
  register const char  *strp,
  int           * const nump,
  const int             min,
  const int             max
  )
{
  register char c;
  register int  num;

  if (strp == NULL || !is_digit(c = *strp))
    return NULL;
  num = 0;
  do {
    num = num * 10 + (c - '0');
    if (num > max)
      return NULL;  /* illegal value */
    c = *++strp;
  } while (is_digit(c));
  if (num < min)
    return NULL;    /* illegal value */
  *nump = num;
  return strp;
}

/*
** Given a pointer into a time zone string, extract a number of seconds,
** in hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the number
** of seconds.
*/
static const char *
getsecs(
  register const char  *strp,
  LONG32          * const secsp
  )
{
  int num;

  /*
  ** `HOURSPERDAY * DAYSPERWEEK - 1' allows quasi-Posix rules like
  ** "M10.4.6/26", which does not conform to Posix,
  ** but which specifies the equivalent of
  ** ``02:00 on the first Sunday on or after 23 Oct''.
  */
  strp = getnum(strp, &num, 0, HOURSPERDAY * DAYSPERWEEK - 1);
  if (strp == NULL)
    return NULL;
  *secsp = (long)(num * SECSPERHOUR);
  if (*strp == ':') {
    ++strp;
    strp = getnum(strp, &num, 0, MINSPERHOUR - 1);
    if (strp == NULL)
      return NULL;
    *secsp += num * SECSPERMIN;
    if (*strp == ':') {
      ++strp;
      /* `SECSPERMIN' allows for leap seconds.  */
      strp = getnum(strp, &num, 0, SECSPERMIN);
      if (strp == NULL)
        return NULL;
      *secsp += num;
    }
  }
  return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/
static const char *
getoffset(
  register const char  *strp,
  LONG32          * const offsetp
  )
{
  register int  neg = 0;

  if (*strp == '-') {
    neg = 1;
    ++strp;
  } else if (*strp == '+')
    ++strp;
  strp = getsecs(strp, offsetp);
  if (strp == NULL)
    return NULL;    /* illegal time */
  if (neg)
    *offsetp = -*offsetp;
  return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time].  See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/
static const char *
getrule(
  const char *strp,
  register struct rule * const rulep
  )
{
  if (*strp == 'J') {
    /*
    ** Julian day.
    */
    rulep->r_type = JULIAN_DAY;
    ++strp;
    strp = getnum(strp, &rulep->r_day, 1, DAYSPERNYEAR);
  } else if (*strp == 'M') {
    /*
    ** Month, week, day.
    */
    rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
    ++strp;
    strp = getnum(strp, &rulep->r_mon, 1, MONSPERYEAR);
    if (strp == NULL)
      return NULL;
    if (*strp++ != '.')
      return NULL;
    strp = getnum(strp, &rulep->r_week, 1, 5);
    if (strp == NULL)
      return NULL;
    if (*strp++ != '.')
      return NULL;
    strp = getnum(strp, &rulep->r_day, 0, DAYSPERWEEK - 1);
  } else if (is_digit(*strp)) {
    /*
    ** Day of year.
    */
    rulep->r_type = DAY_OF_YEAR;
    strp = getnum(strp, &rulep->r_day, 0, DAYSPERLYEAR - 1);
  } else  return NULL;    /* invalid format */
  if (strp == NULL)
    return NULL;
  if (*strp == '/') {
    /*
    ** Time specified.
    */
    ++strp;
    strp = getsecs(strp, &rulep->r_time);
  } else  rulep->r_time = 2 * SECSPERHOUR;  /* default = 2:00:00 */
  return strp;
}

static int
tzload(register const char *name, register struct state * const sp)
{
#ifndef NO_ZONEINFO_FILES
  register const char * p;
  register int    i;
  register int    fid;

  if (name == NULL && (name = TZDEFAULT) == NULL)
    return -1;

  {
    register int  doaccess;
    /*
    ** Section 4.9.1 of the C standard says that
    ** "FILENAME_MAX expands to an integral constant expression
    ** that is the size needed for an array of char large enough
    ** to hold the longest file name string that the implementation
    ** guarantees can be opened."
    */
    char    fullname[FILENAME_MAX + 1];

    if (name[0] == ':')
      ++name;
    doaccess = name[0] == '/';
    if (!doaccess) {
      if ((p = TZDIR) == NULL)
        return -1;
      if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
        return -1;
      (void) strcpy(fullname, p); /* XXX strcpy is safe */
      (void) strcat(fullname, "/"); /* XXX strcat is safe */
      (void) strcat(fullname, name);  /* XXX strcat is safe */
      /*
      ** Set doaccess if '.' (as in "../") shows up in name.
      */
      if (strchr(name, '.') != NULL)
        doaccess = TRUE;
      name = fullname;
    }
    if (doaccess && access(name, R_OK) != 0)
      return -1;
    /*
    * XXX potential security problem here if user of a set-id
    * program has set TZ (which is passed in as name) here,
    * and uses a race condition trick to defeat the access(2)
    * above.
    */
    if ((fid = open(name, OPEN_MODE)) == -1)
      return -1;
  }
  {
    struct tzhead * tzhp;
    union {
      struct tzhead tzhead;
      char    buf[sizeof *sp + sizeof *tzhp];
    } u;
    int   ttisstdcnt;
    int   ttisgmtcnt;

    i = read(fid, u.buf, sizeof u.buf);
    if (close(fid) != 0)
      return -1;
    ttisstdcnt = (int) detzcode(u.tzhead.tzh_ttisstdcnt);
    ttisgmtcnt = (int) detzcode(u.tzhead.tzh_ttisgmtcnt);
    sp->leapcnt = (int) detzcode(u.tzhead.tzh_leapcnt);
    sp->timecnt = (int) detzcode(u.tzhead.tzh_timecnt);
    sp->typecnt = (int) detzcode(u.tzhead.tzh_typecnt);
    sp->charcnt = (int) detzcode(u.tzhead.tzh_charcnt);
    p = u.tzhead.tzh_charcnt + sizeof u.tzhead.tzh_charcnt;
    if (sp->leapcnt < 0 || sp->leapcnt > TZ_MAX_LEAPS ||
        sp->typecnt <= 0 || sp->typecnt > TZ_MAX_TYPES ||
        sp->timecnt < 0 || sp->timecnt > TZ_MAX_TIMES ||
        sp->charcnt < 0 || sp->charcnt > TZ_MAX_CHARS ||
        (ttisstdcnt != sp->typecnt && ttisstdcnt != 0) ||
        (ttisgmtcnt != sp->typecnt && ttisgmtcnt != 0))
      return -1;
    if (i - (p - u.buf) < sp->timecnt * 4 + /* ats */
        sp->timecnt +     /* types */
        sp->typecnt * (4 + 2) +   /* ttinfos */
        sp->charcnt +     /* chars */
        sp->leapcnt * (4 + 4) +   /* lsinfos */
        ttisstdcnt +      /* ttisstds */
        ttisgmtcnt)     /* ttisgmts */
      return -1;
    for (i = 0; i < sp->timecnt; ++i) {
      sp->ats[i] = detzcode(p);
      p += 4;
    }
    for (i = 0; i < sp->timecnt; ++i) {
      sp->types[i] = (unsigned char) *p++;
      if (sp->types[i] >= sp->typecnt)
        return -1;
    }
    for (i = 0; i < sp->typecnt; ++i) {
      register struct ttinfo *  ttisp;

      ttisp = &sp->ttis[i];
      ttisp->tt_gmtoff = detzcode(p);
      p += 4;
      ttisp->tt_isdst = (unsigned char) *p++;
      if (ttisp->tt_isdst != 0 && ttisp->tt_isdst != 1)
        return -1;
      ttisp->tt_abbrind = (unsigned char) *p++;
      if (ttisp->tt_abbrind < 0 ||
          ttisp->tt_abbrind > sp->charcnt)
        return -1;
    }
    for (i = 0; i < sp->charcnt; ++i)
      sp->chars[i] = *p++;
    sp->chars[i] = '\0';  /* ensure '\0' at end */
    for (i = 0; i < sp->leapcnt; ++i) {
      register struct lsinfo *  lsisp;

      lsisp = &sp->lsis[i];
      lsisp->ls_trans = detzcode(p);
      p += 4;
      lsisp->ls_corr = detzcode(p);
      p += 4;
    }
    for (i = 0; i < sp->typecnt; ++i) {
      register struct ttinfo *  ttisp;

      ttisp = &sp->ttis[i];
      if (ttisstdcnt == 0)
        ttisp->tt_ttisstd = FALSE;
      else {
        ttisp->tt_ttisstd = *p++;
        if (ttisp->tt_ttisstd != TRUE &&
            ttisp->tt_ttisstd != FALSE)
          return -1;
      }
    }
    for (i = 0; i < sp->typecnt; ++i) {
      register struct ttinfo *  ttisp;

      ttisp = &sp->ttis[i];
      if (ttisgmtcnt == 0)
        ttisp->tt_ttisgmt = FALSE;
      else {
        ttisp->tt_ttisgmt = *p++;
        if (ttisp->tt_ttisgmt != TRUE &&
            ttisp->tt_ttisgmt != FALSE)
          return -1;
      }
    }
  }
  return 0;
#else   /* ! NO_ZONEINFO_FILES */
  return -1;
#endif
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 UTC, in a year, the
** year, a rule, and the offset from UTC at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/
static
time_t
transtime(
  const time_t              janfirst,
  const int                 year,
  const struct rule * const rulep,
  const LONG32                offset
  )
{
  register int  leapyear;
  register time_t value;
  register int  i;
  int   d, m1, yy0, yy1, yy2, dow;

  INITIALIZE(value);
  leapyear = isleap(year);
  switch (rulep->r_type) {

    case JULIAN_DAY:
      /*
    ** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
    ** years.
    ** In non-leap years, or if the day number is 59 or less, just
    ** add SECSPERDAY times the day number-1 to the time of
    ** January 1, midnight, to get the day.
    */
      value = janfirst + (rulep->r_day - 1) * SECSPERDAY;
      if (leapyear && rulep->r_day >= 60)
        value += SECSPERDAY;
      break;

    case DAY_OF_YEAR:
      /*
    ** n - day of year.
    ** Just add SECSPERDAY times the day number to the time of
    ** January 1, midnight, to get the day.
    */
      value = janfirst + rulep->r_day * SECSPERDAY;
      break;

    case MONTH_NTH_DAY_OF_WEEK:
      /*
    ** Mm.n.d - nth "dth day" of month m.
    */
      value = janfirst;
      for (i = 0; i < rulep->r_mon - 1; ++i)
        value += mon_lengths[leapyear][i] * SECSPERDAY;

      /*
    ** Use Zeller's Congruence to get day-of-week of first day of
    ** month.
    */
      m1 = (rulep->r_mon + 9) % 12 + 1;
      yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
      yy1 = yy0 / 100;
      yy2 = yy0 % 100;
      dow = ((26 * m1 - 2) / 10 +
             1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
      if (dow < 0)
        dow += DAYSPERWEEK;

      /*
    ** "dow" is the day-of-week of the first day of the month.  Get
    ** the day-of-month (zero-origin) of the first "dow" day of the
    ** month.
    */
      d = rulep->r_day - dow;
      if (d < 0)
        d += DAYSPERWEEK;
      for (i = 1; i < rulep->r_week; ++i) {
        if (d + DAYSPERWEEK >=
            mon_lengths[leapyear][rulep->r_mon - 1])
          break;
        d += DAYSPERWEEK;
      }

      /*
    ** "d" is the day-of-month (zero-origin) of the day we want.
    */
      value += d * SECSPERDAY;
      break;
  }

  /*
  ** "value" is the Epoch-relative time of 00:00:00 UTC on the day in
  ** question.  To get the Epoch-relative time of the specified local
  ** time on that day, add the transition time and the current offset
  ** from UTC.
  */
  return value + rulep->r_time + offset;
}

/*
** Given a POSIX section 8-style TZ string, fill in the rule tables as
** appropriate.
*/
static int
tzparse(
  const char    *       name,
  struct state  * const sp,
  const int             lastditch
  )
{
  const char     *stdname;
  const char     *dstname;
  size_t          stdlen;
  size_t          dstlen;
  LONG32            stdoffset;
  LONG32            dstoffset;
  time_t         *atp;
  unsigned char  *typep;
  char           *cp;
  int             load_result;

  dstname = NULL;
  stdname = name;
  if (lastditch) {
    stdlen = strlen(name);  /* length of standard zone name */
    name += stdlen;
    if (stdlen >= sizeof sp->chars)
      stdlen = (sizeof sp->chars) - 1;
    stdoffset = 0;
  } else {
    name = getzname(name);
    stdlen = name - stdname;
    if (stdlen < 3)
      return -1;
    if (*name == '\0')
      return -1;
    name = getoffset(name, &stdoffset);
    if (name == NULL)
      return -1;
  }
  load_result = tzload(TZDEFRULES, sp);
  if (load_result != 0)
    sp->leapcnt = 0;    /* so, we're off a little */
  if (*name != '\0') {
    dstname = name;
    name = getzname(name);
    dstlen = name - dstname;  /* length of DST zone name */
    if (dstlen < 3)
      return -1;
    if (*name != '\0' && *name != ',' && *name != ';') {
      name = getoffset(name, &dstoffset);
      if (name == NULL)
        return -1;
    } else  dstoffset = stdoffset - SECSPERHOUR;
    if (*name == '\0' && load_result != 0)
      name = TZDEFRULESTRING;
    if (*name == ',' || *name == ';') {
      struct rule start;
      struct rule end;
      register int  year;
      register time_t janfirst;
      time_t    starttime;
      time_t    endtime;

      ++name;
      if ((name = getrule(name, &start)) == NULL)
        return -1;
      if (*name++ != ',')
        return -1;
      if ((name = getrule(name, &end)) == NULL)
        return -1;
      if (*name != '\0')
        return -1;
      sp->typecnt = 2;  /* standard time and DST */
      /*
      ** Two transitions per year, from EPOCH_YEAR to 2037.
      */
      sp->timecnt = 2 * (2037 - EPOCH_YEAR + 1);
      if (sp->timecnt > TZ_MAX_TIMES)
        return -1;
      sp->ttis[0].tt_gmtoff = -dstoffset;
      sp->ttis[0].tt_isdst = 1;
      sp->ttis[0].tt_abbrind = (int)stdlen + 1;
      sp->ttis[1].tt_gmtoff = -stdoffset;
      sp->ttis[1].tt_isdst = 0;
      sp->ttis[1].tt_abbrind = 0;
      atp = sp->ats;
      typep = sp->types;
      janfirst = 0;
      for (year = EPOCH_YEAR; year <= 2037; ++year) {
        starttime = transtime(janfirst, year, &start,
                              stdoffset);
        endtime = transtime(janfirst, year, &end,
                            dstoffset);
        if (starttime > endtime) {
          *atp++ = endtime;
          *typep++ = 1; /* DST ends */
          *atp++ = starttime;
          *typep++ = 0; /* DST begins */
        } else {
          *atp++ = starttime;
          *typep++ = 0; /* DST begins */
          *atp++ = endtime;
          *typep++ = 1; /* DST ends */
        }
        janfirst += year_lengths[isleap(year)] *
          SECSPERDAY;
      }
    } else {
      register LONG32 theirstdoffset;
      register LONG32 theiroffset;
      register int  i;
      register int  j;

      if (*name != '\0')
        return -1;
      /*
      ** Initial values of theirstdoffset
      */
      theirstdoffset = 0;
      for (i = 0; i < sp->timecnt; ++i) {
        j = sp->types[i];
        if (!sp->ttis[j].tt_isdst) {
          theirstdoffset =
            -sp->ttis[j].tt_gmtoff;
          break;
        }
      }
      /*
      ** Initially we're assumed to be in standard time.
      */
      theiroffset = theirstdoffset;
      /*
      ** Now juggle transition times and types
      ** tracking offsets as you do.
      */
      for (i = 0; i < sp->timecnt; ++i) {
        j = sp->types[i];
        sp->types[i] = (unsigned char)sp->ttis[j].tt_isdst;
        if (sp->ttis[j].tt_ttisgmt) {
          /* No adjustment to transition time */
        } else {
          /*
          ** If summer time is in effect, and the
          ** transition time was not specified as
          ** standard time, add the summer time
          ** offset to the transition time;
          ** otherwise, add the standard time
          ** offset to the transition time.
          */
          /*
          ** Transitions from DST to DDST
          ** will effectively disappear since
          ** POSIX provides for only one DST
          ** offset.
          */
          sp->ats[i] += stdoffset -
            theirstdoffset;
        }
        theiroffset = -sp->ttis[j].tt_gmtoff;
        if (!sp->ttis[j].tt_isdst)
          theirstdoffset = theiroffset;
      }
      /*
      ** Finally, fill in ttis.
      ** ttisstd and ttisgmt need not be handled.
      */
      sp->ttis[0].tt_gmtoff = -stdoffset;
      sp->ttis[0].tt_isdst = FALSE;
      sp->ttis[0].tt_abbrind = 0;
      sp->ttis[1].tt_gmtoff = -dstoffset;
      sp->ttis[1].tt_isdst = TRUE;
      sp->ttis[1].tt_abbrind = (int)stdlen + 1;
      sp->typecnt = 2;
    }
  } else {
    dstlen = 0;
    sp->typecnt = 1;    /* only standard time */
    sp->timecnt = 0;
    sp->ttis[0].tt_gmtoff = -stdoffset;
    sp->ttis[0].tt_isdst = 0;
    sp->ttis[0].tt_abbrind = 0;
  }
  sp->charcnt = (int)stdlen + 1;
  if (dstlen != 0)
    sp->charcnt += (int)dstlen + 1;
  if ((size_t) sp->charcnt > sizeof sp->chars)
    return -1;
  cp = sp->chars;
  (void) strncpy(cp, stdname, stdlen);
  cp += stdlen;
  *cp++ = '\0';
  if (dstlen != 0) {
    (void) strncpy(cp, dstname, dstlen);
    *(cp + dstlen) = '\0';
  }
  return 0;
}

void
gmtload(struct state * const sp)
{
  if (tzload(gmt, sp) != 0)
    (void) tzparse(gmt, sp, TRUE);
}

static void
tzsetwall(void)
{
  if (lcl_is_set < 0)
    return;
  lcl_is_set = -1;

  if (lclptr == NULL) {
    lclptr = (struct state *) malloc(sizeof *lclptr);
    if (lclptr == NULL) {
      settzname();  /* all we can do */
      return;
    }
  }
  if (tzload((char *) NULL, lclptr) != 0)
    gmtload(lclptr);
  settzname();
}

void
tzset(void)
{
  register const char * name;

  name = getenv("TZ");
  if (name == NULL) {
    tzsetwall();
    return;
  }

  if (lcl_is_set > 0 && strcmp(lcl_TZname, name) == 0)
    return;
  lcl_is_set = strlen(name) < sizeof lcl_TZname;
  if (lcl_is_set)
    (void)strncpyX(lcl_TZname, name, sizeof(lcl_TZname));

  if (lclptr == NULL) {
    lclptr = (struct state *) malloc(sizeof *lclptr);
    if (lclptr == NULL) {
      settzname();  /* all we can do */
      return;
    }
  }
  if (*name == '\0') {
    /*
    ** User wants it fast rather than right.
    */
    lclptr->leapcnt = 0;    /* so, we're off a little */
    lclptr->timecnt = 0;
    lclptr->typecnt = 0;
    lclptr->ttis[0].tt_isdst = 0;
    lclptr->ttis[0].tt_gmtoff = 0;
    lclptr->ttis[0].tt_abbrind = 0;
    (void)strncpyX(lclptr->chars, gmt, sizeof(lclptr->chars));
  } else if (tzload(name, lclptr) != 0)
    if (name[0] == ':' || tzparse(name, lclptr, FALSE) != 0)
    (void) gmtload(lclptr);
  settzname();
}
