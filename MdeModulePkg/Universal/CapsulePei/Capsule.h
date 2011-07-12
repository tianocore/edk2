/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CAPSULE_PEIM_H_
#define _CAPSULE_PEIM_H_

#include <PiPei.h>
#include <Uefi/UefiSpec.h>

#include <Ppi/Capsule.h>

#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/CapsuleVendor.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PrintLib.h>

//
// We want to avoid using memory at 0 for coalescing, so set a
// min address.
//
#define MIN_COALESCE_ADDR 0x100000
#define MAX_SUPPORT_CAPSULE_NUM 50

//
// This capsule PEIM puts its private data at the start of the
// coalesced capsule. Here's the structure definition.
//
#define EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('C', 'a', 'p', 'D')

typedef struct {
  UINT32  Signature;
  UINTN   CapsuleSize;
} EFI_CAPSULE_PEIM_PRIVATE_DATA;

#define CAPSULE_TEST_SIGNATURE SIGNATURE_32('T', 'E', 'S', 'T')

#endif
