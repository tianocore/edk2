/*	$NetBSD: servent.h,v 1.3 2008/04/28 20:23:00 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

struct servent_data {
        void *db;
	struct servent serv;
	char **aliases;
	size_t maxaliases;
	int flags;
#define	_SV_STAYOPEN	1
#define	_SV_DB		2
#define	_SV_FIRST	4
	char *line;
	void *dummy;
};

#if 0
struct servent	*getservent_r(struct servent *, struct servent_data *);
struct servent	*getservbyname_r(const char *, const char *,
    struct servent *, struct servent_data *);
struct servent	*getservbyport_r(int, const char *,
    struct servent *, struct servent_data *);
void setservent_r(int, struct servent_data *);
void endservent_r(struct servent_data *);
#endif  //  0

int _servent_open(struct servent_data *);
void _servent_close(struct servent_data *);
int _servent_getline(struct servent_data *);
struct servent *_servent_parseline(struct servent_data *, struct servent *);
