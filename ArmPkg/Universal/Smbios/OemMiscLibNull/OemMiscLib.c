/** @file
*  OemMiscLib.c
*
*  Copyright (c) 2018, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2018, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>

#include <Library/OemMiscLib.h>


/** Gets the CPU frequency of the specified processor.

  @param ProcessorIndex Index of the processor to get the frequency for.

  @return               CPU frequency in Hz
**/
EFIAPI
UINTN
OemGetCpuFreq (
  IN UINT8 ProcessorIndex
  )
{
  return 0;
}

/** Gets information about the specified processor and stores it in
    the structures provided.

  @param ProcessorIndex  Index of the processor to get the information for.
  @param ProcessorStatus Processor status.
  @param ProcessorCharacteristics Processor characteritics.
  @param MiscProcessorData        Miscellaneous processor information.

  @return  TRUE on success, FALSE on failure.
**/
EFIAPI
BOOLEAN
OemGetProcessorInformation (
  IN UINTN ProcessorIndex,
  IN OUT PROCESSOR_STATUS_DATA *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTIC_FLAGS *ProcessorCharacteristics,
  IN OUT MISC_PROCESSOR_DATA *MiscProcessorData
  )
{
  return FALSE;
}

/** Gets information about the cache at the specified cache level.

  @param CacheLevel The cache level to get information for.
  @param SmbiosCacheTable The SMBIOS Type7 cache information structure.

  @return TRUE on success, FALSE on failure.
**/
EFIAPI
BOOLEAN
OemGetCacheInformation (
  IN UINT8 CacheLevel,
  IN OUT SMBIOS_TABLE_TYPE7 *SmbiosCacheTable
  )
{
  return TRUE;
}

/** Gets the maximum number of sockets supported by the platform.

  @return The maximum number of sockets.
**/
EFIAPI
UINT8
OemGetProcessorMaxSockets (
  VOID
  )
{
  return 1;
}

/** Gets the type of chassis for the system.

  @param ChassisType The type of the chassis.

  @retval EFI_SUCCESS The chassis type was fetched successfully.
**/
EFI_STATUS
EFIAPI
OemGetChassisType (
  UINT8 *ChassisType
  )
{
  *ChassisType = MiscChassisTypeUnknown;
  return EFI_SUCCESS;
}

/** Returns whether the specified processor is present or not.

  @param ProcessIndex The processor index to check.

  @return TRUE is the processor is present, FALSE otherwise.
**/
EFIAPI
BOOLEAN
OemIsSocketPresent (
  IN UINTN ProcessorIndex
  )
{
  if (ProcessorIndex == 0) {
    return TRUE;
  }

  return FALSE;
}

/** Updates the HII string for the specified field.

  @param mHiiHandle    The HII handle.
  @param TokenToUpdate The string to update.
  @param Offset        The field to get information about.
**/
EFIAPI
VOID
UpdateSmbiosInfo (
  IN EFI_HII_HANDLE mHiiHandle,
  IN EFI_STRING_ID TokenToUpdate,
  IN SMBIOS_HII_STRING_FIELD Offset
  )
{

}
