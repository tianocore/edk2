/** @file
  Utility functions used by the Dp application.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HandleParsingLib.h>

#include <Pi/PiFirmwareFile.h>
#include <Library/DxeServicesLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DevicePath.h>

#include <Guid/Performance.h>

#include "Dp.h"
#include "Literals.h"
#include "DpInternal.h"

/**
  Calculate an event's duration in timer ticks.

  Given the count direction and the event's start and end timer values,
  calculate the duration of the event in timer ticks.  Information for
  the current measurement is pointed to by the parameter.

  If the measurement's start time is 1, it indicates that the developer
  is indicating that the measurement began at the release of reset.
  The start time is adjusted to the timer's starting count before performing
  the elapsed time calculation.

  The calculated duration, in ticks, is the absolute difference between
  the measurement's ending and starting counts.

  @param Measurement   Pointer to a MEASUREMENT_RECORD structure containing
                       data for the current measurement.

  @return              The 64-bit duration of the event.
**/
UINT64
GetDuration (
  IN OUT MEASUREMENT_RECORD  *Measurement
  )
{
  UINT64   Duration;
  BOOLEAN  Error;

  if (Measurement->EndTimeStamp == 0) {
    return 0;
  }

  Duration = Measurement->EndTimeStamp - Measurement->StartTimeStamp;
  Error    = (BOOLEAN)(Duration > Measurement->EndTimeStamp);

  if (Error) {
    DEBUG ((DEBUG_ERROR, ALit_TimerLibError));
    Duration = 0;
  }

  return Duration;
}

/**
  Determine whether the Measurement record is for an EFI Phase.

  The Token and Module members of the measurement record are checked.
  Module must be empty and Token must be one of SEC, PEI, DXE, BDS, or SHELL.

  @param[in]  Measurement A pointer to the Measurement record to test.

  @retval     TRUE        The measurement record is for an EFI Phase.
  @retval     FALSE       The measurement record is NOT for an EFI Phase.
**/
BOOLEAN
IsPhase (
  IN MEASUREMENT_RECORD  *Measurement
  )
{
  BOOLEAN  RetVal;

  RetVal = (BOOLEAN)(
                     ((AsciiStrCmp (Measurement->Token, ALit_SEC) == 0)    ||
                      (AsciiStrCmp (Measurement->Token, ALit_PEI) == 0)    ||
                      (AsciiStrCmp (Measurement->Token, ALit_DXE) == 0)    ||
                      (AsciiStrCmp (Measurement->Token, ALit_BDS) == 0))
                     );
  return RetVal;
}

/**
  Determine whether the Measurement record is for core code.

  @param[in] Measurement  A pointer to the Measurement record to test.

  @retval     TRUE        The measurement record is used for core.
  @retval     FALSE       The measurement record is NOT used for core.

**/
BOOLEAN
IsCorePerf (
  IN MEASUREMENT_RECORD  *Measurement
  )
{
  BOOLEAN  RetVal;

  RetVal = (BOOLEAN)(
                     ((Measurement->Identifier == MODULE_START_ID)            ||
                      (Measurement->Identifier == MODULE_END_ID)              ||
                      (Measurement->Identifier == MODULE_LOADIMAGE_START_ID)  ||
                      (Measurement->Identifier == MODULE_LOADIMAGE_END_ID)    ||
                      (Measurement->Identifier == MODULE_DB_START_ID)         ||
                      (Measurement->Identifier == MODULE_DB_END_ID)           ||
                      (Measurement->Identifier == MODULE_DB_SUPPORT_START_ID) ||
                      (Measurement->Identifier == MODULE_DB_SUPPORT_END_ID)   ||
                      (Measurement->Identifier == MODULE_DB_STOP_START_ID)    ||
                      (Measurement->Identifier == MODULE_DB_STOP_START_ID))
                     );
  return RetVal;
}

/**
  Get the file name portion of the Pdb File Name.

  The portion of the Pdb File Name between the last backslash and
  either a following period or the end of the string is converted
  to Unicode and copied into UnicodeBuffer.  The name is truncated,
  if necessary, to ensure that UnicodeBuffer is not overrun.

  @param[in]  PdbFileName     Pdb file name.
  @param[out] UnicodeBuffer   The resultant Unicode File Name.

**/
VOID
DpGetShortPdbFileName (
  IN  CHAR8   *PdbFileName,
  OUT CHAR16  *UnicodeBuffer
  )
{
  UINTN  IndexA;    // Current work location within an ASCII string.
  UINTN  IndexU;    // Current work location within a Unicode string.
  UINTN  StartIndex;
  UINTN  EndIndex;

  ZeroMem (UnicodeBuffer, (DP_GAUGE_STRING_LENGTH + 1) * sizeof (CHAR16));

  if (PdbFileName == NULL) {
    StrnCpyS (UnicodeBuffer, DP_GAUGE_STRING_LENGTH + 1, L" ", 1);
  } else {
    StartIndex = 0;
    for (EndIndex = 0; PdbFileName[EndIndex] != 0; EndIndex++) {
    }

    for (IndexA = 0; PdbFileName[IndexA] != 0; IndexA++) {
      if ((PdbFileName[IndexA] == '\\') || (PdbFileName[IndexA] == '/')) {
        StartIndex = IndexA + 1;
      }

      if (PdbFileName[IndexA] == '.') {
        EndIndex = IndexA;
      }
    }

    IndexU = 0;
    for (IndexA = StartIndex; IndexA < EndIndex; IndexA++) {
      UnicodeBuffer[IndexU] = (CHAR16)PdbFileName[IndexA];
      IndexU++;
      if (IndexU >= DP_GAUGE_STRING_LENGTH) {
        UnicodeBuffer[DP_GAUGE_STRING_LENGTH] = 0;
        break;
      }
    }
  }
}

/**
  Get a human readable name for an image handle.
  The following methods will be tried orderly:
    1. Image PDB
    2. ComponentName2 protocol
    3. FFS UI section
    4. Image GUID
    5. Image DevicePath
    6. Unknown Driver Name

  @param[in]    Handle

  @post   The resulting Unicode name string is stored in the
          mGaugeString global array.

**/
VOID
DpGetNameFromHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                    Status;
  EFI_LOADED_IMAGE_PROTOCOL     *Image;
  CHAR8                         *PdbFileName;
  EFI_DRIVER_BINDING_PROTOCOL   *DriverBinding;
  EFI_STRING                    StringPtr;
  EFI_DEVICE_PATH_PROTOCOL      *LoadedImageDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_GUID                      *NameGuid;
  CHAR16                        *NameString;
  UINTN                         StringSize;
  CHAR8                         *PlatformLanguage;
  CHAR8                         *BestLanguage;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;

  Image                 = NULL;
  LoadedImageDevicePath = NULL;
  DevicePath            = NULL;

  //
  // Method 1: Get the name string from image PDB
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&Image
                  );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **)&DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (
                      DriverBinding->ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&Image
                      );
    }
  }

  if (!EFI_ERROR (Status)) {
    PdbFileName = PeCoffLoaderGetPdbPointer (Image->ImageBase);

    if (PdbFileName != NULL) {
      DpGetShortPdbFileName (PdbFileName, mGaugeString);
      return;
    }
  }

  //
  // Method 2: Get the name string from ComponentName2 protocol
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Firstly use platform language setting, secondly use driver's first supported language.
    //
    GetVariable2 (L"PlatformLang", &gEfiGlobalVariableGuid, (VOID **)&PlatformLanguage, NULL);
    BestLanguage = GetBestLanguage (
                     ComponentName2->SupportedLanguages,
                     FALSE,
                     (PlatformLanguage != NULL) ? PlatformLanguage : "",
                     ComponentName2->SupportedLanguages,
                     NULL
                     );
    SHELL_FREE_NON_NULL (PlatformLanguage);

    Status = ComponentName2->GetDriverName (
                               ComponentName2,
                               BestLanguage != NULL ? BestLanguage : "en-US",
                               &StringPtr
                               );
    if (!EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (BestLanguage);
      StrnCpyS (mGaugeString, DP_GAUGE_STRING_LENGTH + 1, StringPtr, DP_GAUGE_STRING_LENGTH);
      mGaugeString[DP_GAUGE_STRING_LENGTH] = 0;
      return;
    }
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageDevicePathProtocolGuid,
                  (VOID **)&LoadedImageDevicePath
                  );
  if (!EFI_ERROR (Status) && (LoadedImageDevicePath != NULL)) {
    DevicePath = LoadedImageDevicePath;
  } else if (Image != NULL) {
    DevicePath = Image->FilePath;
  }

  if (DevicePath != NULL) {
    //
    // Try to get image GUID from image DevicePath
    //
    NameGuid = NULL;
    while (!IsDevicePathEndType (DevicePath)) {
      NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)DevicePath);
      if (NameGuid != NULL) {
        break;
      }

      DevicePath = NextDevicePathNode (DevicePath);
    }

    if (NameGuid != NULL) {
      //
      // Try to get the image's FFS UI section by image GUID
      //
      NameString = NULL;
      StringSize = 0;
      Status     = GetSectionFromAnyFv (
                     NameGuid,
                     EFI_SECTION_USER_INTERFACE,
                     0,
                     (VOID **)&NameString,
                     &StringSize
                     );

      if (!EFI_ERROR (Status)) {
        //
        // Method 3. Get the name string from FFS UI section
        //
        StrnCpyS (mGaugeString, DP_GAUGE_STRING_LENGTH + 1, NameString, DP_GAUGE_STRING_LENGTH);
        mGaugeString[DP_GAUGE_STRING_LENGTH] = 0;
        FreePool (NameString);
      } else {
        //
        // Method 4: Get the name string from image GUID
        //
        UnicodeSPrint (mGaugeString, sizeof (mGaugeString), L"%g", NameGuid);
      }

      return;
    } else {
      //
      // Method 5: Get the name string from image DevicePath
      //
      NameString = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
      if (NameString != NULL) {
        StrnCpyS (mGaugeString, DP_GAUGE_STRING_LENGTH + 1, NameString, DP_GAUGE_STRING_LENGTH);
        mGaugeString[DP_GAUGE_STRING_LENGTH] = 0;
        FreePool (NameString);
        return;
      }
    }
  }

  //
  // Method 6: Unknown Driver Name
  //
  StringPtr = HiiGetString (mDpHiiHandle, STRING_TOKEN (STR_DP_ERROR_NAME), NULL);
  ASSERT (StringPtr != NULL);
  StrnCpyS (mGaugeString, DP_GAUGE_STRING_LENGTH + 1, StringPtr, DP_GAUGE_STRING_LENGTH);
  FreePool (StringPtr);
}

/**
  Calculate the Duration in microseconds.

  Duration is multiplied by 1000, instead of Frequency being divided by 1000 or
  multiplying the result by 1000, in order to maintain precision.  Since Duration is
  a 64-bit value, multiplying it by 1000 is unlikely to produce an overflow.

  The time is calculated as (Duration * 1000) / Timer_Frequency.

  @param[in]  Duration   The event duration in timer ticks.

  @return     A 64-bit value which is the Elapsed time in microseconds.
**/
UINT64
DurationInMicroSeconds (
  IN UINT64  Duration
  )
{
  return DivU64x32 (Duration, 1000);
}

/**
  Get index of Measurement Record's match in the CumData array.

  If the Measurement's Token value matches a Token in one of the CumData
  records, the index of the matching record is returned.  The returned
  index is a signed value so that negative values can indicate that
  the Measurement didn't match any entry in the CumData array.

  @param[in]  Measurement A pointer to a Measurement Record to match against the CumData array.

  @retval     <0    Token is not in the CumData array.
  @retval     >=0   Return value is the index into CumData where Token is found.
**/
INTN
GetCumulativeItem (
  IN MEASUREMENT_RECORD  *Measurement
  )
{
  INTN  Index;

  for ( Index = 0; Index < (INTN)NumCum; ++Index) {
    if (AsciiStrCmp (Measurement->Token, CumData[Index].Name) == 0) {
      return Index;  // Exit, we found a match
    }
  }

  // If the for loop exits, Token was not found.
  return -1;   // Indicate failure
}
