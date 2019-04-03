/** @file
The Header file of the Pci Host Bridge Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _PCI_HOST_RESOURCE_H_
#define _PCI_HOST_RESOURCE_H_

#include <PiDxe.h>

#define EFI_RESOURCE_NONEXISTENT  0xFFFFFFFFFFFFFFFFULL
#define EFI_RESOURCE_LESS         0xFFFFFFFFFFFFFFFEULL

typedef struct {
  UINTN   BusBase;
  UINTN   BusLimit;
  UINTN   BusReserve;

  UINT32  Mem32Base;
  UINT32  Mem32Limit;

  UINT64  Mem64Base;
  UINT64  Mem64Limit;

  UINTN   IoBase;
  UINTN   IoLimit;
} PCI_ROOT_BRIDGE_RESOURCE_APERTURE;

typedef enum {
  TypeIo    = 0,
  TypeMem32,
  TypePMem32,
  TypeMem64,
  TypePMem64,
  TypeBus,
  TypeMax
} PCI_RESOURCE_TYPE;

typedef enum {
  ResNone     = 0,
  ResSubmitted,
  ResRequested,
  ResAllocated,
  ResStatusMax
} RES_STATUS;

typedef struct {
  PCI_RESOURCE_TYPE Type;
  UINT64            Base;
  UINT64            Length;
  UINT64            Alignment;
  RES_STATUS        Status;
} PCI_RES_NODE;

#endif
