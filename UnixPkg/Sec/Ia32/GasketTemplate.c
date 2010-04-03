/** @file
  Template file used to create Gasket.S

  This file is built on the command line via gcc GasketTemplate.c -S 
  and it will create GasketTemplate.s and this was used to create 
  Gasket.S. You still have to add the extra stack alignment code to 
  the assembly functions. 

Copyright (c) 2006 - 2009, Intel Corporation<BR>
Portions copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <stdint.h>
#include <sys/stat.h>

typedef int8_t    INT8;
typedef uint8_t   UINT8;
typedef int16_t   INT16;
typedef uint16_t  UINT16;
typedef int32_t   INT32;
typedef uint32_t  UINT32;
typedef int64_t   INT64;
typedef uint64_t  UINT64;
typedef UINT32    UINTN; 


typedef int (*GASKET_VOID) ();
typedef int (*GASKET_UINTN) (UINTN);
typedef int (*GASKET_UINTN_UINTN) (UINTN, UINTN);
typedef int (*GASKET_UINTN_UINTN_UINTN) (UINTN, UINTN, UINTN);
typedef int (*GASKET_UINTN_UINTN_UINTN_UINTN) (UINTN, UINTN, UINTN, UINTN);
typedef int (*GASKET_UINTN_10ARGS) (UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN);
typedef int (*GASKET_UINT64_UINTN) (UINT64, UINTN);
typedef UINT64 (*GASKET_UINTN_UINT64_UINTN) (UINTN, UINT64, UINTN);
typedef int (*GASKET_UINTN_UINT16) (UINTN, UINT16);

int GasketVoid (void *api);
int GasketUintn (void *api, UINTN a);
int GasketUintnUintn (void *api, UINTN a, UINTN b);
int GasketUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c);
int GasketUintnUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c, UINTN d);
int GasketUintn10Args (void *api, UINTN a, UINTN b, UINTN c, UINTN d, UINTN e, UINTN f, UINTN g, UINTN h, UINTN i, UINTN j);
int GasketUint64Uintn (void *api, UINT64 a, UINTN b);
UINT64 GasketUintnUiny64Uintn (void *api, UINTN a, UINT64 b, UINTN c);
int GasketUintnUint16 (void *api, UINTN a, UINT16 b);



int
GasketVoid (void *api)
{
  GASKET_VOID func;
  
  func = (GASKET_VOID)api;
  return func ();
}

int
GasketUintn (void *api, UINTN a)
{
  GASKET_UINTN func;
  
  func = (GASKET_UINTN)api;
  return func (a);
}

int
GasketUintnUintn (void *api, UINTN a, UINTN b)
{
  GASKET_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN)api;
  return func (a, b);
}


int
GasketUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c)
{
  GASKET_UINTN_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN_UINTN)api;
  return func (a, b, c);
}

int
GasketUintnUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c, UINTN d)
{
  GASKET_UINTN_UINTN_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN_UINTN_UINTN)api;
  return func (a, b, c, d);
}

int
GasketUintn10Args (void *api, UINTN a, UINTN b, UINTN c, UINTN d, UINTN e, UINTN f, UINTN g, UINTN h, UINTN i, UINTN j)
{
  GASKET_UINTN_10ARGS func;
  
  func = (GASKET_UINTN_10ARGS)api;
  return func (a, b, c, d, e, f, g, h, i, j);
}


int
GasketUint64Uintn (void *api, UINT64 a, UINTN b)
{
  GASKET_UINT64_UINTN func;
  
  func = (GASKET_UINT64_UINTN)api;
  return func (a, b);
}

UINT64
GasketUintnUiny64Uintn (void *api, UINTN a, UINT64 b, UINTN c)
{
  GASKET_UINTN_UINT64_UINTN func;
  
  func = (GASKET_UINTN_UINT64_UINTN)api;
  return func (a, b, c);
}

int
GasketUintnUint16 (void *api, UINTN a, UINT16 b)
{
  GASKET_UINTN_UINT16 func;
  
  func = (GASKET_UINTN_UINT16)api;
  return func (a, b);
}



