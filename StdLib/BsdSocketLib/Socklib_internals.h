/*
  Definitions for the socket library functions that are used internally.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef _SOCKLIB_INTERNALS_H_
#define _SOCKLIB_INTERNALS_H_

void _sethosthtent (int);
void _endhosthtent (void);
void _sethostdnsent (int);
void _endhostdnsent (void);
void _setnethtent (int);
void _endnethtent (void);
void _setnetdnsent (int);
void _endnetdnsent (void);

struct hostent * _gethostbyhtname (const char *, int);
struct hostent * _gethostbydnsname (const char *, int);
struct hostent * _gethostbynisname (const char *, int);
struct hostent * _gethostbyhtaddr (const char *, int, int);
struct hostent * _gethostbydnsaddr (const char *, int, int);
struct hostent * _gethostbynisaddr (const char *, int, int);
struct netent *  _getnetbyhtname (const char *);
struct netent *  _getnetbydnsname (const char *);
struct netent *  _getnetbynisname (const char *);
struct netent *  _getnetbyhtaddr (unsigned long, int);
struct netent *  _getnetbydnsaddr (unsigned long, int);
struct netent *  _getnetbynisaddr (unsigned long, int);
void _map_v4v6_address (const char *src, char *dst);
void _map_v4v6_hostent (struct hostent *hp, char **bp, int *len);
#endif

