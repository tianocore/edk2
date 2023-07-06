/** @file

  The Header file of the Pci Host Bridge Driver.

Copyright (c) 1999 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCI_HOST_RESOURCE_H_
#define _PCI_HOST_RESOURCE_H_

#include <PiDxe.h>

#define PCI_RESOURCE_LESS  0xFFFFFFFFFFFFFFFEULL

typedef enum {
  TypeIo = 0,
  TypeMem32,
  TypePMem32,
  TypeMem64,
  TypePMem64,
  TypeBus,
  TypeMax
} PCI_RESOURCE_TYPE;

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
#define PCI_RESOURCE_TYPE_ENUM_START  TypeIo
#else
#define PCI_RESOURCE_TYPE_ENUM_START  TypeMem32
#endif

typedef enum {
  ResNone,
  ResSubmitted,
  ResAllocated,
  ResStatusMax
} RES_STATUS;

typedef struct {
  PCI_RESOURCE_TYPE    Type;
  //
  // Base is a host address
  //
  UINT64               Base;
  UINT64               Length;
  UINT64               Alignment;
  RES_STATUS           Status;
} PCI_RES_NODE;

#endif
