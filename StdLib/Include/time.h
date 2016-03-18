/** @file
    The header <time.h> defines two macros, and declares several types and
    functions for manipulating time.  Many functions deal with a calendar time
    that represents the current date (according to the Gregorian calendar) and
    time.  Some functions deal with local time, which is the calendar time
    expressed for some specific time zone, and with Daylight Saving Time, which
    is a temporary change in the algorithm for determining local time.  The local
    time zone and Daylight Saving Time are implementation-defined.

    The macros defined are NULL; and CLOCKS_PER_SEC which expands to an
    expression with type clock_t (described below) that is the number per second
    of the value returned by the clock function.

    The types declared are size_t along with clock_t and time_t which are
    arithmetic types capable of representing times; and struct tm which holds
    the components of a calendar time, called the broken-down time.

    The range and precision of times representable in clock_t and time_t are
    implementation-defined. The tm structure shall contain at least the following
    members, in any order.  The semantics of the members and their normal ranges
    are expressed in the comments.
      - int tm_sec;   // seconds after the minute - [0, 60]
      - int tm_min;   // minutes after the hour - [0, 59]
      - int tm_hour;  // hours since midnight - [0, 23]
      - int tm_mday;  // day of the month - [1, 31]
      - int tm_mon;   // months since January - [0, 11]
      - int tm_year;  // years since 1900
      - int tm_wday;  // days since Sunday - [0, 6]
      - int tm_yday;  // days since January 1 - [0, 365]
      - int tm_isdst; // Daylight Saving Time flag

    The value of tm_isdst is positive if Daylight Saving Time is in effect, zero
    if Daylight Saving Time is not in effect, and negative if the information
    is not available.

    The following macros are defined in this file:<BR>
    @verbatim
      NULL
      CLOCKS_PER_SEC    The number of values per second returned by the clock function.
    @endverbatim

    The following types are defined in this file:<BR>
    @verbatim
      size_t      Unsigned integer type of the result of the sizeof operator.
      clock_t     Arithmetic type capable of representing a time from the clock function.
      time_t      Arithmetic type capable of representing a time.
      struct tm   Holds the components of a calendar time; or broken-down time.
    @endverbatim

    The following functions are declared in this file:<BR>
    @verbatim
      ###############  Time Manipulation Functions
      clock_t       clock     (void);
      double        difftime  (time_t time1, time_t time0);
      time_t        mktime    (struct tm *timeptr);
      time_t        time      (time_t *timer);

      #################  Time Conversion Functions
      char        * asctime   (const struct tm *timeptr);
      char        * ctime     (const time_t *timer);
      struct tm   * gmtime    (const time_t *timer);
      time_t        timegm    (struct tm*);
      struct tm   * localtime (const time_t *timer);
      size_t        strftime  (char * __restrict s, size_t maxsize,
                               const char * __restrict format,
                               const struct tm * __restrict timeptr);
      char        * strptime  (const char *, const char * format, struct tm*);
    @endverbatim

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _TIME_H
#define _TIME_H
#include <sys/EfiCdefs.h>

#define CLOCKS_PER_SEC  __getCPS()

#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

#ifdef _EFI_CLOCK_T
  /** An arithmetic type capable of representing values returned by clock(); **/
  typedef _EFI_CLOCK_T  clock_t;
  #undef _EFI_CLOCK_T
#endif

#ifdef _EFI_TIME_T
  /** An arithmetic type capable of representing values returned as calendar time
      values, such as that returned by mktime();
  **/
  typedef _EFI_TIME_T  time_t;
  #undef _EFI_TIME_T
#endif

/** Value added to tm_year to get the full year value.  TM_YEAR_BASE + 110 --> 2010 **/
#define TM_YEAR_BASE  1900

/** @{
    Values for the tm_wday member of struct tm.
**/
#define TM_SUNDAY     0
#define TM_MONDAY     1
#define TM_TUESDAY    2
#define TM_WEDNESDAY  3
#define TM_THURSDAY   4
#define TM_FRIDAY     5
#define TM_SATURDAY   6
/*@}*/

/** @{
    Values for the tm_mon member of struct tm.
**/
#define TM_JANUARY     0
#define TM_FEBRUARY    1
#define TM_MARCH       2
#define TM_APRIL       3
#define TM_MAY         4
#define TM_JUNE        5
#define TM_JULY        6
#define TM_AUGUST      7
#define TM_SEPTEMBER   8
#define TM_OCTOBER     9
#define TM_NOVEMBER   10
#define TM_DECEMBER   11
/*@}*/

/** A structure holding the components of a calendar time, called the
    broken-down time.  The first nine (9) members are as mandated by the
    C95 standard.  Additional fields have been added for EFI support.
**/
struct tm {
  int     tm_year;      // years since 1900
  int     tm_mon;       // months since January  [0, 11]
  int     tm_mday;      // day of the month  [1, 31]
  int     tm_hour;      // hours since midnight  [0, 23]
  int     tm_min;       // minutes after the hour  [0, 59]
  int     tm_sec;       // seconds after the minute  [0, 60]
  int     tm_wday;      // days since Sunday  [0, 6]
  int     tm_yday;      // days since January 1  [0, 365]
  int     tm_isdst;     // Daylight Saving Time flag
  int     tm_zoneoff;   // EFI TimeZone offset, -1440 to 1440 or 2047
  int     tm_daylight;  // EFI Daylight flags
  UINT32  tm_Nano;      // EFI Nanosecond value
};

/* ###############  Time Manipulation Functions  ########################## */

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
clock_t  clock(void);

/** Compute the difference between two calendar times: time1 - time0.

    @param[in]  time1   An arithmetic calendar time.
    @param[in]  time2   Another arithmetic calendar time.

    @return   The difference between the two times expressed in seconds.
**/
double difftime(time_t time1, time_t time0);

/** Convert a broken-down time into an arithmetic calendar time.

    The mktime function converts the broken-down time, expressed as local time,
    in the structure pointed to by timeptr into a calendar time value with the
    same encoding as that of the values returned by the time function. The
    original values of the tm_wday and tm_yday components of the structure are
    ignored, and the original values of the other components are not
    restricted to the ranges indicated above. On successful completion,
    the values of the tm_wday and tm_yday components of the structure are set
    appropriately, and the other components are set to represent the specified
    calendar time, but with their values forced to the ranges indicated above;
    the final value of tm_mday is not set until tm_mon and tm_year
    are determined.

    @param[in]  timeptr   Pointer to a broken-down time to be converted.

    @return   The mktime function returns the specified calendar time encoded
              as a value of type time_t. If the calendar time cannot be
              represented, the function returns the value (time_t)(-1).
**/
time_t mktime(struct tm *timeptr);

/** The time function determines the current calendar time.

    The encoding of the value is unspecified and undocumented.

    @param[out]   timer   An optional pointer to an object in which to
                          store the calendar time.

    @return   The time function returns the implementation's best approximation
              of the current calendar time. The value (time_t)(-1) is returned
              if the calendar time is not available. If timer is not a null
              pointer, the return value is also assigned to the object it
              points to.
**/
time_t time(time_t *timer);

/* #################  Time Conversion Functions  ########################## */

/** The asctime function converts the broken-down time in the structure pointed
    to by timeptr into a string in the form<BR>
    @verbatim
          Sun Sep 16 01:03:52 1973\n\0
    @endverbatim

    @param[in]  timeptr   A pointer to a broken-down time to convert.

    @return   The asctime function returns a pointer to the string.
**/
char * asctime(const struct tm *timeptr);

/** The ctime function converts the calendar time pointed to by timer to a local
    time in the form of a string. It is equivalent to asctime(localtime(timer))

    @param[in]  timer   Pointer to a calendar time value to convert into a
                        string representation.

    @return   The ctime function returns the pointer returned by the asctime
              function with that broken-down time as argument.
**/
char * ctime(const time_t *timer);

/** The gmtime function converts the calendar time pointed to by timer into a
    broken-down time, expressed as UTC.

    @param[in]  timer   Pointer to a calendar time value to convert into a
                        broken-down time.

    @return   The gmtime function returns a pointer to the broken-down time,
              or a null pointer if the specified time cannot be converted to UTC.
**/
struct tm  * gmtime(const time_t *timer);

/** The timegm function is the opposite of gmtime.

    @param[in]  tm    Pointer to a broken-down time to convert into a
                      calendar time.

    @return   The calendar time expressed as UTC.
**/
time_t timegm(struct tm*);

/** The localtime function converts the calendar time pointed to by timer into
    a broken-down time, expressed as local time.

    @param[in]  timer   Pointer to a calendar time value to be converted.

    @return   The localtime function returns a pointer to the broken-down time,
              or a null pointer if the specified time cannot be converted to
              local time.
**/
struct tm  * localtime(const time_t *timer);

/** The strftime function places characters into the array pointed to by s as
    controlled by the string pointed to by format. The format shall be a
    multibyte character sequence, beginning and ending in its initial shift
    state. The format string consists of zero or more conversion specifiers
    and ordinary multibyte characters. A conversion specifier consists of
    a % character, possibly followed by an E or O modifier character
    (described below), followed by a character that determines the behavior of
    the conversion specifier.

    All ordinary multibyte characters (including the terminating null
    character) are copied unchanged into the array. If copying takes place
    between objects that overlap, the behavior is undefined. No more than
    maxsize characters are placed into the array. 3 Each conversion specifier
    is replaced by appropriate characters as described in the following list.
    The appropriate characters are determined using the LC_TIME category of
    the current locale and by the values of zero or more members of the
    broken-down time structure pointed to by timeptr, as specified in brackets
    in the description. If any of the specified values is outside the normal
    range, the characters stored are unspecified.

    %a is replaced by the locale's abbreviated weekday name. [tm_wday]
    %A is replaced by the locale's full weekday name. [tm_wday]
    %b is replaced by the locale's abbreviated month name. [tm_mon]
    %B is replaced by the locale's full month name. [tm_mon]
    %c is replaced by the locale's appropriate date and time representation.
    %C is replaced by the year divided by 100 and truncated to an integer,
       as a decimal number (00-99). [tm_year]
    %d is replaced by the day of the month as a decimal number (01-31). [tm_mday]
    %D is equivalent to "%m/%d/%y". [tm_mon, tm_mday, tm_year]
    %e is replaced by the day of the month as a decimal number (1-31);
       a single digit is preceded by a space. [tm_mday]
    %F is equivalent to "%Y-%m-%d" (the ISO 8601 date format).
       [tm_year, tm_mon, tm_mday]
    %g is replaced by the last 2 digits of the week-based year (see below) as
       a decimal number (00-99). [tm_year, tm_wday, tm_yday]
    %G is replaced by the week-based year (see below) as a decimal number
       (e.g., 1997). [tm_year, tm_wday, tm_yday]
    %h is equivalent to "%b". [tm_mon]
    %H is replaced by the hour (24-hour clock) as a decimal number (00-23). [tm_hour]
    %I is replaced by the hour (12-hour clock) as a decimal number (01-12). [tm_hour]
    %j is replaced by the day of the year as a decimal number (001-366). [tm_yday]
    %m is replaced by the month as a decimal number (01-12). [tm_mon]
    %M is replaced by the minute as a decimal number (00-59). [tm_min]
    %n is replaced by a new-line character.
    %p is replaced by the locale's equivalent of the AM/PM designations
       associated with a 12-hour clock. [tm_hour]
    %r is replaced by the locale's 12-hour clock time. [tm_hour, tm_min, tm_sec]
    %R is equivalent to "%H:%M". [tm_hour, tm_min]
    %S is replaced by the second as a decimal number (00-60). [tm_sec]
    %t is replaced by a horizontal-tab character.
    %T is equivalent to "%H:%M:%S" (the ISO 8601 time format).
       [tm_hour, tm_min, tm_sec]
    %u is replaced by the ISO 8601 weekday as a decimal number (1-7),
       where Monday is 1. [tm_wday]
    %U is replaced by the week number of the year (the first Sunday as the
       first day of week 1) as a decimal number (00-53). [tm_year, tm_wday, tm_yday]
    %V is replaced by the ISO 8601 week number (see below) as a decimal number
       (01-53). [tm_year, tm_wday, tm_yday]
    %w is replaced by the weekday as a decimal number (0-6), where Sunday is 0.
       [tm_wday]
    %W is replaced by the week number of the year (the first Monday as the
       first day of week 1) as a decimal number (00-53). [tm_year, tm_wday, tm_yday]
    %x is replaced by the locale's appropriate date representation.
    %X is replaced by the locale's appropriate time representation.
    %y is replaced by the last 2 digits of the year as a decimal
       number (00-99). [tm_year]
    %Y is replaced by the year as a decimal number (e.g., 1997). [tm_year]
    %z is replaced by the offset from UTC in the ISO 8601 format "-0430"
       (meaning 4 hours 30 minutes behind UTC, west of Greenwich), or by no
       characters if no time zone is determinable. [tm_isdst]
    %Z is replaced by the locale's time zone name or abbreviation, or by no
       characters if no time zone is determinable. [tm_isdst]
    %% is replaced by %.

    Some conversion specifiers can be modified by the inclusion of an E or O
    modifier character to indicate an alternative format or specification.
    If the alternative format or specification does not exist for the current
    locale, the modifier is ignored. %Ec is replaced by the locale's
    alternative date and time representation.

    %EC is replaced by the name of the base year (period) in the locale's
        alternative representation.
    %Ex is replaced by the locale's alternative date representation.
    %EX is replaced by the locale's alternative time representation.
    %Ey is replaced by the offset from %EC (year only) in the locale's
        alternative representation.
    %EY is replaced by the locale's full alternative year representation.
    %Od is replaced by the day of the month, using the locale's alternative
        numeric symbols (filled as needed with leading zeros, or with leading
        spaces if there is no alternative symbol for zero).
    %Oe is replaced by the day of the month, using the locale's alternative
        numeric symbols (filled as needed with leading spaces).
    %OH is replaced by the hour (24-hour clock), using the locale's
        alternative numeric symbols.
    %OI is replaced by the hour (12-hour clock), using the locale's
        alternative numeric symbols.
    %Om is replaced by the month, using the locale's alternative numeric symbols.
    %OM is replaced by the minutes, using the locale's alternative numeric symbols.
    %OS is replaced by the seconds, using the locale's alternative numeric symbols.
    %Ou is replaced by the ISO 8601 weekday as a number in the locale's
        alternative representation, where Monday is 1.
    %OU is replaced by the week number, using the locale's alternative numeric symbols.
    %OV is replaced by the ISO 8601 week number, using the locale's alternative
        numeric symbols.
    %Ow is replaced by the weekday as a number, using the locale's alternative
        numeric symbols.
    %OW is replaced by the week number of the year, using the locale's
        alternative numeric symbols.
    %Oy is replaced by the last 2 digits of the year, using the locale's
        alternative numeric symbols.

    %g, %G, and %V give values according to the ISO 8601 week-based year. In
    this system, weeks begin on a Monday and week 1 of the year is the week
    that includes January 4th, which is also the week that includes the first
    Thursday of the year, and is also the first week that contains at least
    four days in the year. If the first Monday of January is the 2nd, 3rd, or
    4th, the preceding days are part of the last week of the preceding year;
    thus, for Saturday 2nd January 1999, %G is replaced by 1998 and %V is
    replaced by 53. If December 29th, 30th, or 31st is a Monday, it and any
    following days are part of week 1 of the following year. Thus, for Tuesday
    30th December 1997, %G is replaced by 1998 and %V is replaced by 01.

    If a conversion specifier is not one of the above, the behavior is undefined.

    In the "C" locale, the E and O modifiers are ignored and the replacement
    strings for the following specifiers are:
      %a the first three characters of %A.
      %A one of "Sunday", "Monday", ... , "Saturday".
      %b the first three characters of %B.
      %B one of "January", "February", ... , "December".
      %c equivalent to "%a %b %e %T %Y".
      %p one of "AM" or "PM".
      %r equivalent to "%I:%M:%S %p".
      %x equivalent to "%m/%d/%y".
      %X equivalent to %T.
      %Z implementation-defined.

    @param  s         Pointer to the buffer in which to store the result.
    @param  maxsize   Maximum number of characters to put into buffer s.
    @param  format    Format string, as described above.
    @param  timeptr   Pointer to a broken-down time structure containing the
                      time to format.

    @return   If the total number of resulting characters including the
              terminating null character is not more than maxsize, the
              strftime function returns the number of characters placed into
              the array pointed to by s not including the terminating null
              character. Otherwise, zero is returned and the contents of the
              array are indeterminate.
**/
size_t strftime( char * __restrict s, size_t maxsize,
                      const char * __restrict format,
                      const struct tm * __restrict timeptr);

char *strptime(const char *, const char * format, struct tm*);


/* #################  Implementation Functions  ########################### */

clock_t __getCPS(void);

#endif  /* _TIME_H */
