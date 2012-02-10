/*
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions copyright (c) 1999, 2000
 * Intel Corporation.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *    This product includes software developed by Intel Corporation and
 *    its contributors.
 * 
 * 4. Neither the name of Intel Corporation or its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/* Import. */

#include <arpa/nameser.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SPRINTF(x) ((size_t)sprintf x)

/* Forward. */

static int	fmt1(int t, char s, char **buf, size_t *buflen);

/* Macros. */

#define T(x) if ((x) < 0) return (-1); else (void)NULL

/* Public. */

int
ns_format_ttl(u_long src, char *dst, size_t dstlen) {
	char *odst = dst;
	int secs, mins, hours, days, weeks, x;
	char *p;

	secs =  (int)(src % 60);  src /= 60;
	mins =  (int)(src % 60);  src /= 60;
	hours = (int)(src % 24);  src /= 24;
	days =  (int)(src % 7);   src /= 7;
	weeks = (int)src;         src = 0;

	x = 0;
	if (weeks) {
		T(fmt1(weeks, 'W', &dst, &dstlen));
		x++;
	}
	if (days) {
		T(fmt1(days, 'D', &dst, &dstlen));
		x++;
	}
	if (hours) {
		T(fmt1(hours, 'H', &dst, &dstlen));
		x++;
	}
	if (mins) {
		T(fmt1(mins, 'M', &dst, &dstlen));
		x++;
	}
	if (secs || !(weeks || days || hours || mins)) {
		T(fmt1(secs, 'S', &dst, &dstlen));
		x++;
	}

	if (x > 1) {
		int ch;

		for (p = odst; (ch = *p) != '\0'; p++)
			if (isascii(ch) && isupper(ch))
				*p = (char)( tolower(ch));
	}

	return ((int)(dst - odst));
}

int
ns_parse_ttl(const char *src, u_long *dst) {
	u_long ttl, tmp;
	int ch, digits, dirty;

	ttl = 0;
	tmp = 0;
	digits = 0;
	dirty = 0;
	while ((ch = *src++) != '\0') {
		if (!isascii(ch) || !isprint(ch))
			goto einval;
		if (isdigit(ch)) {
			tmp *= 10;
			tmp += (ch - '0');
			digits++;
			continue;
		}
		if (digits == 0)
			goto einval;
		if (islower(ch))
			ch = toupper(ch);
		switch (ch) {
		case 'W':  tmp *= 7;
		case 'D':  tmp *= 24;
		case 'H':  tmp *= 60;
		case 'M':  tmp *= 60;
		case 'S':  break;
		default:   goto einval;
		}
		ttl += tmp;
		tmp = 0;
		digits = 0;
		dirty = 1;
	}
	if (digits > 0) {
		if (dirty)
			goto einval;
		else
			ttl += tmp;
	}
	*dst = ttl;
	return (0);

 einval:
	errno = EINVAL;
	return (-1);
}

/* Private. */

static int
fmt1(int t, char s, char **buf, size_t *buflen) {
	char tmp[50];
	size_t len;

	len = SPRINTF((tmp, "%d%c", t, s));
	if (len + 1 > *buflen)
		return (-1);
	strcpy(*buf, tmp);
	*buf += len;
	*buflen -= len;
	return (0);
}
