/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscOnboardDeviceFunction.c
  
Abstract: 

  Onboard device information boot time changes.
  Misc. subclass type 8.
  SMBIOS type 10.

--*/

#include "MiscSubclassDriver.h"
#include "winntio/winntio.h"
#include "winntthunk/winntthunk.h"

#pragma pack(1)

typedef struct _VENDOR_DEVICE {
  EFI_DEVICE_PATH_PROTOCOL  Platform;
  EFI_GUID                  PlatformGuid;
  EFI_DEVICE_PATH_PROTOCOL  Device;
  EFI_GUID                  DeviceGuid;
  UINT8                     DeviceData[4];
  EFI_DEVICE_PATH_PROTOCOL  End;

} VENDOR_DEVICE;
#pragma pack()

MISC_SUBCLASS_TABLE_FUNCTION (
  MiscOnboardDeviceVideo
  )
/*++
Description:

  This function makes boot time changes to the contents of the
  MiscOnboardDevice structure.

Parameters:

  RecordType
    Type of record to be processed from the Data Table.
    mMiscSubclassDataTable[].RecordType

  RecordLen
    Size of static RecordData from the Data Table.
    mMiscSubclassDataTable[].RecordLen

  RecordData
    Pointer to copy of RecordData from the Data Table.  Changes made
    to this copy will be written to the Data Hub but will not alter
    the contents of the static Data Table.

  LogRecordData
    Set *LogRecordData to TRUE to log RecordData to Data Hub.
    Set *LogRecordData to FALSE when there is no more data to log.

Returns:

  EFI_SUCCESS
    All parameters were valid and *RecordData and *LogRecordData have
    been set.

  EFI_UNSUPPORTED
    Unexpected RecordType value.

  EFI_INVALID_PARAMETER
    One of the following parameter conditions was true:
      RecordLen was zero.
      RecordData was NULL.
      LogRecordData was NULL.
--*/
{
  STATIC VENDOR_DEVICE  mVideoDevicePath = {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      0x14
    },
    EFI_WIN_NT_THUNK_PROTOCOL_GUID,
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      0x18
    },
    EFI_WIN_NT_UGA_GUID,
    0,
    0,
    0,
    0,
    END
  };

  STATIC BOOLEAN        Done = FALSE;

  //
  // First check for invalid parameters.
  //
  if (RecordLen == 0 || RecordData == NULL || LogRecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Then check for unsupported RecordType.
  //
  if (RecordType != EFI_MISC_ONBOARD_DEVICE_DATA_RECORD_NUMBER) {
    return EFI_UNSUPPORTED;
  }
  //
  // Is this the first time through this function?
  //
  if (!Done) {
    //
    // Yes, this is the first time.  Inspect/Change the contents of the
    // RecordData structure.
    //
    //
    // Any time changes?
    //
    // %%TBD
    //
    // Set Done flag to TRUE for next pass through this function.
    // Set *LogRecordData to TRUE so data will get logged to Data Hub.
    //
    switch (((EFI_MISC_ONBOARD_DEVICE_DATA *) RecordData)->OnBoardDeviceDescription) {
    case STR_MISC_ONBOARD_DEVICE_VIDEO_DESCRIPTION:
      {
        CopyMem (
          &((EFI_MISC_ONBOARD_DEVICE_DATA *) RecordData)->OnBoardDevicePath,
          &mVideoDevicePath,
          GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mVideoDevicePath)
          );
        *RecordLen = *RecordLen - sizeof (EFI_DEVICE_PATH_PROTOCOL) + GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) &mVideoDevicePath);
      }
      break;
    }

    Done            = TRUE;
    *LogRecordData  = TRUE;
  } else {
    //
    // No, this is the second time.  Reset the state of the Done flag
    // to FALSE and tell the data logger that there is no more data
    // to be logged for this record type.  If any memory allocations
    // were made by earlier passes, they must be released now.
    //
    Done            = FALSE;
    *LogRecordData  = FALSE;
  }

  return EFI_SUCCESS;
}

/* eof - MiscOnboardDeviceFunction.c */
