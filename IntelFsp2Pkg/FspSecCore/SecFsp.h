/** @file

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SEC_FSP_H_
#define _SEC_FSP_H_

#include <PiPei.h>
#include <FspEas.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FspCommonLib.h>
#include <Library/FspSecPlatformLib.h>

#define FSP_MCUD_SIGNATURE  SIGNATURE_32 ('M', 'C', 'U', 'D')
#define FSP_PER0_SIGNATURE  SIGNATURE_32 ('P', 'E', 'R', '0')

/**

  Calculate the FSP IDT gate descriptor.

  @param[in] IdtEntryTemplate     IDT gate descriptor template.

  @return                     FSP specific IDT gate descriptor.

**/
UINT64
FspGetExceptionHandler(
  IN  UINT64  IdtEntryTemplate
  );

/**

  Initialize the FSP global data region.
  It needs to be done as soon as possible after the stack is setup.

  @param[in,out] PeiFspData             Pointer of the FSP global data.
  @param[in]     BootLoaderStack        BootLoader stack.
  @param[in]     ApiIdx                 The index of the FSP API.

**/
VOID
FspGlobalDataInit (
  IN OUT  FSP_GLOBAL_DATA    *PeiFspData,
  IN UINT32                   BootLoaderStack,
  IN UINT8                    ApiIdx
  );


/**

  Adjust the FSP data pointers after the stack is migrated to memory.

  @param[in] OffsetGap             The offset gap between the old stack and the new stack.

**/
VOID
FspDataPointerFixUp (
  IN UINT32   OffsetGap
  );


/**
  This interface returns the base address of FSP binary.

  @return   FSP binary base address.

**/
UINT32
EFIAPI
AsmGetFspBaseAddress (
  VOID
  );

/**
  This interface gets FspInfoHeader pointer

  @return   FSP binary base address.

**/
UINT32
EFIAPI
AsmGetFspInfoHeader (
  VOID
  );

#endif
