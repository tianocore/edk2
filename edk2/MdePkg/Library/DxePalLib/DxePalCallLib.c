/** @file
  PAL Library Class implementation whose PAL entry is collected
  from HOB or SAL System Table.

  Copyright (c) 2007 - 2008 Intel Corporation. All rights reserved
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

  Module Name:  DxePalCallLib.c

**/

#include <PiDxe.h>
#include <ItaniumFamilyCpuDxe.h>
#include <IndustryStandard/Sal.h>

#include <Guid/PalEntryHob.h>
#include <Guid/SalSystemTable.h>

#include <Library/PalCallLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>


BOOLEAN mPalCallAddressHob = FALSE;
BOOLEAN mPalCallAddressSal = FALSE;
UINT64  mPalCallAddress;

/**
  Makes a PAL procedure call.

  This is a wrapper function to make a PAL procedure call.  Based on the Index value,
  this API will make static or stacked PAL call. Architected procedures may be designated
  as required or optional.  If a PAL procedure is specified as optional, a unique return
  code of 0xFFFFFFFFFFFFFFFF is returned in the Status field of the PAL_CALL_RETURN structure.
  This indicates that the procedure is not present in this PAL implementation.  It is the
  caller's responsibility to check for this return code after calling any optional PAL
  procedure. No parameter checking is performed on the 4 input parameters, but there are
  some common rules that the caller should follow when making a PAL call.  Any address
  passed to PAL as buffers for return parameters must be 8-byte aligned.  Unaligned addresses
  may cause undefined results.  For those parameters defined as reserved or some fields
  defined as reserved must be zero filled or the invalid argument return value may be
  returned or undefined result may occur during the execution of the procedure.
  This function is only available on IPF.

  @param  Index  The PAL procedure Index number.
  @param  Arg2   The 2nd parameter for PAL procedure calls.
  @param  Arg3   The 3rd parameter for PAL procedure calls.
  @param  Arg4   The 4th parameter for PAL procedure calls.

  @return Structure returned from the PAL Call procedure, including the status and return value.

**/
PAL_CALL_RETURN
EFIAPI
PalCall (
  IN UINT64                  Index,
  IN UINT64                  Arg2,
  IN UINT64                  Arg3,
  IN UINT64                  Arg4
  )
{
  EFI_HOB_GUID_TYPE              *GuidHob;
  EFI_STATUS                     Status;
  SAL_ST_ENTRY_POINT_DESCRIPTOR  *SalStEntryDes;
  SAL_SYSTEM_TABLE_HEADER        *SalSystemTable;


  if (!mPalCallAddressHob) {
    //
    // Collect PAL entry from HOBs
    //
    GuidHob = GetFirstGuidHob (&gPalEntryHobGuid);
    ASSERT (GuidHob != NULL);

    mPalCallAddress = *((EFI_PHYSICAL_ADDRESS *) GET_GUID_HOB_DATA (GuidHob));
    ASSERT (mPalCallAddress != 0);

    mPalCallAddressHob = TRUE;
  }

  if (!mPalCallAddressSal) {
    Status = EfiGetSystemConfigurationTable (
               &gEfiSalSystemTableGuid,
               (VOID **) &SalSystemTable
               );

    if (!EFI_ERROR (Status)) {
      //
      // Move the SAL System Table point to the first Entry
      // Due to the SAL Entry is in ascending order with the Entry type,
      // the type 0 Entry should be the first if exist.
      //
      SalStEntryDes = (SAL_ST_ENTRY_POINT_DESCRIPTOR *)(SalSystemTable + 1);

      //
      // Assure the SAL ENTRY Type is 0
      //
      ASSERT (SalStEntryDes->Type == EFI_SAL_ST_ENTRY_POINT);

      if (SalStEntryDes->PalProcEntry != 0) {
        mPalCallAddress = SalStEntryDes->PalProcEntry;
        mPalCallAddressSal = TRUE;
      }
    }
  }

  return AsmPalCall (mPalCallAddress, Index, Arg2, Arg3, Arg4);
}
