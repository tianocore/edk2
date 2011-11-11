/**
  Definitions and Implementation for <time.h>.

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
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/TimerLib.h>
#include  <Library/BaseLib.h>
#include  <Library/UefiRuntimeServicesTableLib.h>
//#include  <Library/UefiRuntimeLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <limits.h>
#include  <time.h>
#include  <reentrant.h>
#include  "tzfile.h"
#include  "TimeVals.h"
#include  <MainData.h>
#include  <extern.h>      // Library/include/extern.h: Private to implementation

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
// Keep compiler quiet about casting from function to data pointers
#pragma warning ( disable : 4054 )
#endif  /* defined(_MSC_VER) */

/* #######################  Private Data  ################################# */

#if 0
static EFI_TIME TimeBuffer;

  static  UINT16   MonthOffs[12] = {
     00,
     31,   59,   90,  120,
    151,  181,  212,  243,
    273,  304,  334
  };
  static  clock_t   y2kOffs = 730485;
#endif

const int  mon_lengths[2][MONSPERYEAR] = {
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

const int  year_lengths[2] = {
  DAYSPERNYEAR, DAYSPERLYEAR
};


static const char *wday_name[7] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *mon_name[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int    gmt_is_set;

/* ###############  Implementation Functions  ############################ */
// Forward reference
static void
localsub(const time_t * const timep, const long   offset, struct tm * const tmp);

clock_t
__getCPS(void)
{
  return gMD->ClocksPerSecond;
}

static void
timesub(
  const time_t        * const timep,
  const long                  offset,
  const struct state  * const sp,
        struct tm     * const tmp
  )
{
  const struct lsinfo *  lp;
  time_t /*INTN*/     days;
  time_t /*INTN*/     rem;
  time_t /*INTN*/      y;
  int      yleap;
  const int *    ip;
  time_t /*INTN*/     corr;
  int      hit;
  int      i;

  corr = 0;
  hit = 0;
#ifdef ALL_STATE
  i = (sp == NULL) ? 0 : sp->leapcnt;
#endif /* defined ALL_STATE */
#ifndef ALL_STATE
  i = sp->leapcnt;
#endif /* State Farm */
  while (--i >= 0) {
    lp = &sp->lsis[i];
    if (*timep >= lp->ls_trans) {
      if (*timep == lp->ls_trans) {
        hit = ((i == 0 && lp->ls_corr > 0) ||
               lp->ls_corr > sp->lsis[i - 1].ls_corr);
        if (hit)
          while (i > 0                                                &&
                 sp->lsis[i].ls_trans == sp->lsis[i - 1].ls_trans + 1 &&
                 sp->lsis[i].ls_corr  == sp->lsis[i - 1].ls_corr  + 1 )
          {
            ++hit;
            --i;
          }
      }
      corr = lp->ls_corr;
      break;
    }
  }
  days = *timep / SECSPERDAY;
  rem = *timep % SECSPERDAY;
  rem += (offset - corr);
  while (rem < 0) {
    rem += SECSPERDAY;
    --days;
  }
  while (rem >= SECSPERDAY) {
    rem -= SECSPERDAY;
    ++days;
  }
  tmp->tm_hour = (int) (rem / SECSPERHOUR);
  rem = rem % SECSPERHOUR;
  tmp->tm_min = (int) (rem / SECSPERMIN);
  /*
  ** A positive leap second requires a special
  ** representation.  This uses "... ??:59:60" et seq.
  */
  tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
  tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYSPERWEEK);
  if (tmp->tm_wday < 0)
    tmp->tm_wday += DAYSPERWEEK;
  y = EPOCH_YEAR;
  while (days < 0 || days >= (LONG32) year_lengths[yleap = isleap(y)]) {
    time_t /*INTN*/  newy;

    newy = (y + days / DAYSPERNYEAR);
    if (days < 0)
      --newy;
    days -= (newy - y) * DAYSPERNYEAR +
      LEAPS_THRU_END_OF(newy - 1) -
      LEAPS_THRU_END_OF(y - 1);
    y = newy;
  }
  tmp->tm_year = (int)(y - TM_YEAR_BASE);
  tmp->tm_yday = (int) days;
  ip = mon_lengths[yleap];
  for (tmp->tm_mon = 0; days >= (LONG32) ip[tmp->tm_mon]; ++(tmp->tm_mon))
    days = days - (LONG32) ip[tmp->tm_mon];
  tmp->tm_mday = (int) (days + 1);
  tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
  tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
}

/* ###############  Time Manipulation Functions  ########################## */

/**
**/
double
difftime(time_t time1, time_t time0)
{
  return (double)(time1 - time0);
}

/*
** Adapted from code provided by Robert Elz, who writes:
**  The "best" way to do mktime I think is based on an idea of Bob
**  Kridle's (so its said...) from a long time ago.
**  [kridle@xinet.com as of 1996-01-16.]
**  It does a binary search of the time_t space.  Since time_t's are
**  just 32 bits, its a max of 32 iterations (even at 64 bits it
**  would still be very reasonable).
*/

#ifndef WRONG
#define WRONG (-1)
#endif /* !defined WRONG */

/*
** Simplified normalize logic courtesy Paul Eggert (eggert@twinsun.com).
*/

static int
increment_overflow(int * number, int delta)
{
  int number0;

  number0 = *number;
  *number += delta;
  return (*number < number0) != (delta < 0);
}

static int
normalize_overflow(int * const tensptr, int * const unitsptr, const int base)
{
  register int  tensdelta;

  tensdelta = (*unitsptr >= 0)  ?
              (*unitsptr / base) : (-1 - (-1 - *unitsptr) / base);
  *unitsptr -= tensdelta * base;
  return increment_overflow(tensptr, tensdelta);
}

static int
tmcomp(const struct tm * const atmp, const struct tm * const btmp)
{
  register int  result;

  if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
      (result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
      (result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
      (result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
      (result = (atmp->tm_min - btmp->tm_min)) == 0)
    result = atmp->tm_sec - btmp->tm_sec;
  return result;
}

static time_t
time2sub(
  struct tm * const tmp,
  void (* const funcp)(const time_t*, long, struct tm*),
  const long offset,
  int * const okayp,
  const int do_norm_secs
  )
{
  register const struct state * sp;
  register int                  dir;
  register int                  bits;
  register int                  i, j ;
  register int                  saved_seconds;
  time_t                        newt;
  time_t                        t;
  struct tm                     yourtm, mytm;

  *okayp = FALSE;
  yourtm = *tmp;    // Create a copy of tmp
  if (do_norm_secs) {
    if (normalize_overflow(&yourtm.tm_min, &yourtm.tm_sec,
                           SECSPERMIN))
      return WRONG;
  }
  if (normalize_overflow(&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR))
    return WRONG;
  if (normalize_overflow(&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY))
    return WRONG;
  if (normalize_overflow(&yourtm.tm_year, &yourtm.tm_mon, MONSPERYEAR))
    return WRONG;
  /*
  ** Turn yourtm.tm_year into an actual year number for now.
  ** It is converted back to an offset from TM_YEAR_BASE later.
  */
  if (increment_overflow(&yourtm.tm_year, TM_YEAR_BASE))
    return WRONG;
  while (yourtm.tm_mday <= 0) {
    if (increment_overflow(&yourtm.tm_year, -1))
      return WRONG;
    i = yourtm.tm_year + (1 < yourtm.tm_mon);
    yourtm.tm_mday += year_lengths[isleap(i)];
  }
  while (yourtm.tm_mday > DAYSPERLYEAR) {
    i = yourtm.tm_year + (1 < yourtm.tm_mon);
    yourtm.tm_mday -= year_lengths[isleap(i)];
    if (increment_overflow(&yourtm.tm_year, 1))
      return WRONG;
  }
  for ( ; ; ) {
    i = mon_lengths[isleap(yourtm.tm_year)][yourtm.tm_mon];
    if (yourtm.tm_mday <= i)
      break;
    yourtm.tm_mday -= i;
    if (++yourtm.tm_mon >= MONSPERYEAR) {
      yourtm.tm_mon = 0;
      if (increment_overflow(&yourtm.tm_year, 1))
        return WRONG;
    }
  }
  if (increment_overflow(&yourtm.tm_year, -TM_YEAR_BASE))
    return WRONG;
  if (yourtm.tm_sec >= 0 && yourtm.tm_sec < SECSPERMIN)
    saved_seconds = 0;
  else if (yourtm.tm_year + TM_YEAR_BASE < EPOCH_YEAR) {
    /*
    ** We can't set tm_sec to 0, because that might push the
    ** time below the minimum representable time.
    ** Set tm_sec to 59 instead.
    ** This assumes that the minimum representable time is
    ** not in the same minute that a leap second was deleted from,
    ** which is a safer assumption than using 58 would be.
    */
    if (increment_overflow(&yourtm.tm_sec, 1 - SECSPERMIN))
      return WRONG;
    saved_seconds = yourtm.tm_sec;
    yourtm.tm_sec = SECSPERMIN - 1;
  } else {
    saved_seconds = yourtm.tm_sec;
    yourtm.tm_sec = 0;
  }
  /*
  ** Divide the search space in half
  ** (this works whether time_t is signed or unsigned).
  */
  bits = TYPE_BIT(time_t) - 1;
  /*
  ** Set t to the midpoint of our binary search.
  **
  ** If time_t is signed, then 0 is just above the median,
  ** assuming two's complement arithmetic.
  ** If time_t is unsigned, then (1 << bits) is just above the median.
  */
  t = TYPE_SIGNED(time_t) ? 0 : (((time_t) 1) << bits);
  for ( ; ; ) {
    (*funcp)(&t, offset, &mytm);    // Convert t to broken-down time in mytm
    dir = tmcomp(&mytm, &yourtm);   // Is mytm larger, equal, or less than yourtm?
    if (dir != 0) {                 // If mytm != yourtm...
      if (bits-- < 0)                   // If we have exhausted all the bits..
        return WRONG;                       // Return that we failed
      if (bits < 0)                     // If on the last bit...
        --t; /* may be needed if new t is minimal */
      else if (dir > 0)                 // else if mytm > yourtm...
        t -= ((time_t) 1) << bits;          // subtract half the remaining time-space
      else  t += ((time_t) 1) << bits;      // otherwise add half the remaining time-space
      continue;                     // Repeat for the next half
    }
    if (yourtm.tm_isdst < 0 || mytm.tm_isdst == yourtm.tm_isdst)
      break;
    /*
    ** Right time, wrong type.
    ** Hunt for right time, right type.
    ** It's okay to guess wrong since the guess
    ** gets checked.
    */
    /*
    ** The (void *) casts are the benefit of SunOS 3.3 on Sun 2's.
    */
    sp = (const struct state *)
      (((void *) funcp == (void *) localsub) ?
       lclptr : gmtptr);
#ifdef ALL_STATE
    if (sp == NULL)
      return WRONG;
#endif /* defined ALL_STATE */
    for (i = sp->typecnt - 1; i >= 0; --i) {
      if (sp->ttis[i].tt_isdst != yourtm.tm_isdst)
        continue;
      for (j = sp->typecnt - 1; j >= 0; --j) {
        if (sp->ttis[j].tt_isdst == yourtm.tm_isdst)
          continue;
        newt = t + sp->ttis[j].tt_gmtoff -
          sp->ttis[i].tt_gmtoff;
        (*funcp)(&newt, offset, &mytm);
        if (tmcomp(&mytm, &yourtm) != 0)
          continue;
        if (mytm.tm_isdst != yourtm.tm_isdst)
          continue;
        /*
        ** We have a match.
        */
        t = newt;
        goto label;
      }
    }
    return WRONG;
  }
  label:
  newt = t + saved_seconds;
  if ((newt < t) != (saved_seconds < 0))
    return WRONG;
  t = newt;
  (*funcp)(&t, offset, tmp);
  *okayp = TRUE;
  return t;
}

time_t
time2(struct tm * const tmp, void (* const funcp)(const time_t*, long, struct tm*),
      const long offset, int * const okayp)
{
  time_t  t;

  /*
  ** First try without normalization of seconds
  ** (in case tm_sec contains a value associated with a leap second).
  ** If that fails, try with normalization of seconds.
  */
  t = time2sub(tmp, funcp, offset, okayp, FALSE);
  return *okayp ? t : time2sub(tmp, funcp, offset, okayp, TRUE);
}

static time_t
time1(
  struct tm * const tmp,
  void (* const funcp)(const time_t *, long, struct tm *),
  const long offset
  )
{
  register time_t               t;
  register const struct state * sp;
  register int                  samei, otheri;
  register int                  sameind, otherind;
  register int                  i;
  register int                  nseen;
  int                           seen[TZ_MAX_TYPES];
  int                           types[TZ_MAX_TYPES];
  int                           okay;

  if (tmp->tm_isdst > 1)
    tmp->tm_isdst = 1;
  t = time2(tmp, funcp, offset, &okay);
#ifdef PCTS
  /*
  ** PCTS code courtesy Grant Sullivan (grant@osf.org).
  */
  if (okay)
    return t;
  if (tmp->tm_isdst < 0)
    tmp->tm_isdst = 0;  /* reset to std and try again */
#endif /* defined PCTS */
#ifndef PCTS
  if (okay || tmp->tm_isdst < 0)
    return t;
#endif /* !defined PCTS */
  /*
  ** We're supposed to assume that somebody took a time of one type
  ** and did some math on it that yielded a "struct tm" that's bad.
  ** We try to divine the type they started from and adjust to the
  ** type they need.
  */
  /*
  ** The (void *) casts are the benefit of SunOS 3.3 on Sun 2's.
  */
  sp = (const struct state *) (((void *) funcp == (void *) localsub) ?
                               lclptr : gmtptr);
#ifdef ALL_STATE
  if (sp == NULL)
    return WRONG;
#endif /* defined ALL_STATE */
  for (i = 0; i < sp->typecnt; ++i)
    seen[i] = FALSE;
  nseen = 0;
  for (i = sp->timecnt - 1; i >= 0; --i)
    if (!seen[sp->types[i]]) {
    seen[sp->types[i]] = TRUE;
    types[nseen++] = sp->types[i];
    }
    for (sameind = 0; sameind < nseen; ++sameind) {
      samei = types[sameind];
      if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
        continue;
      for (otherind = 0; otherind < nseen; ++otherind) {
        otheri = types[otherind];
        if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
          continue;
        tmp->tm_sec += (int)(sp->ttis[otheri].tt_gmtoff -
                             sp->ttis[samei].tt_gmtoff);
        tmp->tm_isdst = !tmp->tm_isdst;
        t = time2(tmp, funcp, offset, &okay);
        if (okay)
          return t;
        tmp->tm_sec -= (int)(sp->ttis[otheri].tt_gmtoff -
                             sp->ttis[samei].tt_gmtoff);
        tmp->tm_isdst = !tmp->tm_isdst;
      }
    }
    return WRONG;
}

/** The mktime function converts the broken-down time, expressed as local time,
    in the structure pointed to by timeptr into a calendar time value with the
    same encoding as that of the values returned by the time function.  The
    original values of the tm_wday and tm_yday components of the structure are
    ignored, and the original values of the other components are not restricted
    to the ranges indicated above.  Thus, a positive or zero value for tm_isdst
    causes the mktime function to presume initially that Daylight Saving Time,
    respectively, is or is not in effect for the specified time. A negative
    value causes it to attempt to determine whether Daylight Saving Time is in
    effect for the specified time.  On successful completion, the values of the
    tm_wday and tm_yday components of the structure are set appropriately, and
    the other components are set to represent the specified calendar time, but
    with their values forced to the ranges indicated above; the final value of
    tm_mday is not set until tm_mon and tm_year are determined.

    @return   The mktime function returns the specified calendar time encoded
              as a value of type time_t.  If the calendar time cannot be
              represented, the function returns the value (time_t)(-1).
**/
time_t
mktime(struct tm *timeptr)
{
  /* From NetBSD */
  time_t result;

  rwlock_wrlock(&lcl_lock);
  tzset();
  result = time1(timeptr, &localsub, 0L);
  rwlock_unlock(&lcl_lock);
  return (result);
}

/** The time function determines the current calendar time.  The encoding of
    the value is unspecified.

    @return   The time function returns the implementation's best approximation
              to the current calendar time.  The value (time_t)(-1) is returned
              if the calendar time is not available.  If timer is not a null
              pointer, the return value is also assigned to the object it
              points to.
**/
time_t
time(time_t *timer)
{
  time_t      CalTime;
  EFI_STATUS  Status;
  EFI_TIME   *ET;
  struct tm  *BT;

  ET = &gMD->TimeBuffer;
  BT = &gMD->BDTime;

  // Get EFI Time
  Status = gRT->GetTime( ET, NULL);
//  Status = EfiGetTime( ET, NULL);
  EFIerrno = Status;
  if( Status != RETURN_SUCCESS) {
    return (time_t)-1;
  }

  // Convert EFI time to broken-down time.
  Efi2Tm( ET, BT);

  // Convert to time_t
  CalTime  =  mktime(&gMD->BDTime);

  if( timer != NULL) {
    *timer = CalTime;
  }
  return CalTime;   // Return calendar time in microseconds
}

/** The clock function determines the processor time used.

    @return   The clock function returns the implementation's best
              approximation to the processor time used by the program since the
              beginning of an implementation-defined era related only to the
              program invocation.  To determine the time in seconds, the value
              returned by the clock function should be divided by the value of
              the macro CLOCKS_PER_SEC.  If the processor time used is not
              available or its value cannot be represented, the function
              returns the value (clock_t)(-1).
**/
clock_t
clock(void)
{
  clock_t   retval;
  time_t    temp;

  temp = time(NULL);
  retval = ((clock_t)((UINT32)temp)) - gMD->AppStartTime;
  return retval;
}

/* #################  Time Conversion Functions  ########################## */
/*
    Except for the strftime function, these functions each return a pointer to
    one of two types of static objects: a broken-down time structure or an
    array of char.  Execution of any of the functions that return a pointer to
    one of these object types may overwrite the information in any object of
    the same type pointed to by the value returned from any previous call to
    any of them.  The implementation shall behave as if no other library
    functions call these functions.
*/

/** The asctime function converts the broken-down time in the structure pointed
    to by timeptr into a string in the form
      Sun Sep 16 01:03:52 1973\n\0
    using the equivalent of the following algorithm.

      char *asctime(const struct tm *timeptr)
      {
        static const char wday_name[7][3] = {
          "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
        };
        static const char mon_name[12][3] = {
          "Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        static char result[26];
        sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
                wday_name[timeptr->tm_wday],
                mon_name[timeptr->tm_mon],
                timeptr->tm_mday, timeptr->tm_hour,
                timeptr->tm_min, timeptr->tm_sec,
                1900 + timeptr->tm_year);
        return result;
      }
    @return   The asctime function returns a pointer to the string.
**/
char *
asctime(const struct tm *timeptr)
{
  register const char * wn;
  register const char * mn;

  if (timeptr->tm_wday < 0 || timeptr->tm_wday >= DAYSPERWEEK)
    wn = "???";
  else  wn = wday_name[timeptr->tm_wday];
  if (timeptr->tm_mon < 0 || timeptr->tm_mon >= MONSPERYEAR)
    mn = "???";
  else  mn = mon_name[timeptr->tm_mon];
  /*
  ** The X3J11-suggested format is
  **  "%.3s %.3s%3d %02.2d:%02.2d:%02.2d %d\n"
  ** Since the .2 in 02.2d is ignored, we drop it.
  */
  (void)snprintf(gMD->ASasctime,
                 sizeof (char[ASCTIME_BUFLEN]),
                 "%.3s %.3s%3d %02d:%02d:%02d %d\r\n",    // explicit CRLF for EFI
                 wn, mn,
                 timeptr->tm_mday, timeptr->tm_hour,
                 timeptr->tm_min, timeptr->tm_sec,
                 TM_YEAR_BASE + timeptr->tm_year);
  return gMD->ASasctime;
}

/**
**/
char *
ctime(const time_t *timer)
{
  return asctime(localtime(timer));
}

/*
** gmtsub is to gmtime as localsub is to localtime.
*/
void
gmtsub(
  const time_t * const  timep,
  const long            offset,
  struct tm    * const  tmp
  )
{
#ifdef _REENTRANT
  static mutex_t gmt_mutex = MUTEX_INITIALIZER;
#endif

  mutex_lock(&gmt_mutex);
  if (!gmt_is_set) {
    gmt_is_set = TRUE;
#ifdef ALL_STATE
    gmtptr = (struct state *) malloc(sizeof *gmtptr);
    if (gmtptr != NULL)
#endif /* defined ALL_STATE */
      gmtload(gmtptr);
  }
  mutex_unlock(&gmt_mutex);
  timesub(timep, offset, gmtptr, tmp);
#ifdef TM_ZONE
  /*
  ** Could get fancy here and deliver something such as
  ** "UTC+xxxx" or "UTC-xxxx" if offset is non-zero,
  ** but this is no time for a treasure hunt.
  */
  if (offset != 0)
    tmp->TM_ZONE = (__aconst char *)__UNCONST(wildabbr);
  else {
#ifdef ALL_STATE
    if (gmtptr == NULL)
      tmp->TM_ZONE = (__aconst char *)__UNCONST(gmt);
    else  tmp->TM_ZONE = gmtptr->chars;
#endif /* defined ALL_STATE */
#ifndef ALL_STATE
    tmp->TM_ZONE = gmtptr->chars;
#endif /* State Farm */
  }
#endif /* defined TM_ZONE */
}

/**
**/
struct tm *
gmtime(const time_t *timer)
{
  gmtsub(timer, 0L, &gMD->BDTime);
  return &gMD->BDTime;
}

static void
localsub(const time_t * const timep, const long   offset, struct tm * const tmp)
{
  register struct state *   sp;
  register const struct ttinfo *  ttisp;
  register int      i;
  const time_t      t = *timep;

  sp = lclptr;
#ifdef ALL_STATE
  if (sp == NULL) {
    gmtsub(timep, offset, tmp);
    return;
  }
#endif /* defined ALL_STATE */
  if (sp->timecnt == 0 || t < sp->ats[0]) {
    i = 0;
    while (sp->ttis[i].tt_isdst)
      if (++i >= sp->typecnt) {
        i = 0;
        break;
      }
  } else {
    for (i = 1; i < sp->timecnt; ++i)
      if (t < sp->ats[i])
      break;
    i = sp->types[i - 1];
  }
  ttisp = &sp->ttis[i];
  /*
  ** To get (wrong) behavior that's compatible with System V Release 2.0
  ** you'd replace the statement below with
  **  t += ttisp->tt_gmtoff;
  **  timesub(&t, 0L, sp, tmp);
  */
  timesub(&t, ttisp->tt_gmtoff, sp, tmp);
  tmp->tm_isdst = ttisp->tt_isdst;
  tzname[tmp->tm_isdst] = &sp->chars[ttisp->tt_abbrind];
#ifdef TM_ZONE
  tmp->TM_ZONE = &sp->chars[ttisp->tt_abbrind];
#endif /* defined TM_ZONE */
}

/**
**/
struct tm *
localtime(const time_t *timer)
{
  tzset();
  localsub(timer, 0L, &gMD->BDTime);
  return &gMD->BDTime;
}
