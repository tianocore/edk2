/** @file
  Template file used to create Gasket.S

  This file is built on the command line via gcc GasketTemplate.c -S 
  and it will create GasketTemplate.s and this was used to create 
  Gasket.S. This builds code for Unix ABI on both sides. To convert 
  to EFI ABI will require changing the code by hand

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
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
typedef UINT64    UINTN; 


typedef UINTN (*GASKET_VOID) ();
typedef UINTN (*GASKET_UINTN) (UINTN);
typedef UINTN (*GASKET_UINT64) (UINT64);
typedef UINTN (*GASKET_UINTN_UINTN) (UINTN, UINTN);
typedef UINTN (*GASKET_UINTN_UINTN_UINTN) (UINTN, UINTN, UINTN);
typedef UINTN (*GASKET_UINTN_UINTN_UINTN_UINTN) (UINTN, UINTN, UINTN, UINTN);
typedef UINTN (*GASKET_UINTN_10ARGS) (UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN);
typedef UINTN (*GASKET_UINT64_UINTN) (UINT64, UINTN);
typedef UINT64 (*GASKET_UINTN_UINT64_UINTN) (UINTN, UINT64, UINTN);
typedef UINTN (*GASKET_UINTN_UINT16) (UINTN, UINT16);

UINTN GasketVoid (void *api);
UINTN GasketUintn (void *api, UINTN a);
UINTN GasketUintnUintn (void *api, UINTN a, UINTN b);
UINTN GasketUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c);
UINTN GasketUintnUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c, UINTN d);
UINTN GasketUintn10Args (void *api, UINTN a, UINTN b, UINTN c, UINTN d, UINTN e, UINTN f, UINTN g, UINTN h, UINTN i, UINTN j);
UINTN GasketUint64Uintn (void *api, UINT64 a, UINTN b);
UINT64 GasketUintnUiny64Uintn (void *api, UINTN a, UINT64 b, UINTN c);
UINTN GasketUintnUint16 (void *api, UINTN a, UINT16 b);



UINTN
GasketVoid (void *api)
{
  GASKET_VOID func;
  
  func = (GASKET_VOID)api;
  return func ();
}

UINTN
GasketUintn (void *api, UINTN a)
{
  GASKET_UINTN func;
  
  func = (GASKET_UINTN)api;
  return func (a);
}

UINTN
GasketUintnUintn (void *api, UINTN a, UINTN b)
{
  GASKET_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN)api;
  return func (a, b);
}


UINTN
GasketUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c)
{
  GASKET_UINTN_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN_UINTN)api;
  return func (a, b, c);
}

UINTN
GasketUintnUintnUintnUintn (void *api, UINTN a, UINTN b, UINTN c, UINTN d)
{
  GASKET_UINTN_UINTN_UINTN_UINTN func;
  
  func = (GASKET_UINTN_UINTN_UINTN_UINTN)api;
  return func (a, b, c, d);
}

UINTN
GasketUintn10Args (void *api, UINTN a, UINTN b, UINTN c, UINTN d, UINTN e, UINTN f, UINTN g, UINTN h, UINTN i, UINTN j)
{
  GASKET_UINTN_10ARGS func;
  
  func = (GASKET_UINTN_10ARGS)api;
  return func (a, b, c, d, e, f, g, h, i, j);
}


UINTN
GasketUint64Uintn (void *api, UINT64 a, UINTN b)
{
  GASKET_UINT64_UINTN func;
  
  func = (GASKET_UINT64_UINTN)api;
  return func (a, b);
}

UINT64
GasketUintnUint64Uintn (void *api, UINTN a, UINT64 b, UINTN c)
{
  GASKET_UINTN_UINT64_UINTN func;
  
  func = (GASKET_UINTN_UINT64_UINTN)api;
  return func (a, b, c);
}

UINTN
GasketUintnUint16 (void *api, UINTN a, UINT16 b)
{
  GASKET_UINTN_UINT16 func;
  
  func = (GASKET_UINTN_UINT16)api;
  return func (a, b);
}

void
ReverseGasketUint64 (void *api, UINT64 a)
{
  GASKET_UINT64 func;
  
  func = (GASKET_UINT64)api;
  func (a);
  return;
}




