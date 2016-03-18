/** @file 
  timegm implementation

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  * Copyright (c) 1987, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

static char *sccsid = "from: @(#)ctime.c	5.26 (Berkeley) 2/23/91";


 * This implementation of mktime is lifted straight from the NetBSD (BSD 4.4)
 * version.  I modified it slightly to divorce it from the internals of the
 * ctime library.  Thus this version can't use details of the internal
 * timezone state file to figure out strange unnormalized struct tm values,
 * as might result from someone doing date math on the tm struct then passing
 * it to mktime.
 *
 * It just does as well as it can at normalizing the tm input, then does a
 * binary search of the time space using the system's localtime() function.
 *
 * The original binary search was defective in that it didn't consider the
 * setting of tm_isdst when comparing tm values, causing the search to be
 * flubbed for times near the dst/standard time changeover.  The original
 * code seems to make up for this by grubbing through the timezone info
 * whenever the binary search barfed.  Since I don't have that luxury in
 * portable code, I have to take care of tm_isdst in the comparison routine.
 * This requires knowing how many minutes offset dst is from standard time.
 *
 * So, if you live somewhere in the world where dst is not 60 minutes offset,
 * and your vendor doesn't supply mktime(), you'll have to edit this variable
 * by hand.  Sorry about that.

	$NetBSD: mktime.c,v 1.4 2006/06/11 19:34:10 kardel Exp $
**/

#include  <LibConfig.h>
#include  <time.h>

/*
  This funciton is in Time.c, which has a different license than timegm.
*/
time_t 
time2(struct tm * const tmp, void (* const funcp)(const time_t*, long, struct tm*),
      const long offset, int * const okayp);

/*
  This funciton is in Time.c, which has a different license than timegm.
*/
void
gmtsub(
  const time_t * const  timep,
  const long            offset,
  struct tm    * const  tmp
  );

#ifndef WRONG
#define WRONG (-1)
#endif /* !defined WRONG */

/*
  Convert a tm structure to a GMT based time_t.
*/
time_t timegm( struct tm * tmp )
{
	register time_t			t;
	int				okay;

	tmp->tm_isdst = 0;
	t = time2(tmp, gmtsub, 0, &okay);
	if (okay || tmp->tm_isdst < 0)
		return t;

	return WRONG;
}