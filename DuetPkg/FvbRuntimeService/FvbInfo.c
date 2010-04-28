/**@file
Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FvbInfo.c

Abstract:

  Defines data structure that is the volume header found.These data is intent
  to decouple FVB driver with FV header.

**/
#include "FileIo.h"
#include "FlashLayout.h"

typedef struct {
  UINT64                      FvLength;
  EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
  EFI_FV_BLOCK_MAP_ENTRY      End;
} EFI_FVB_MEDIA_INFO;

#define FVB_MEDIA_BLOCK_SIZE    FIRMWARE_BLOCK_SIZE
#define RECOVERY_BOIS_BLOCK_NUM FIRMWARE_BLOCK_NUMBER
#define SYSTEM_NV_BLOCK_NUM     2

EFI_FVB_MEDIA_INFO  mPlatformFvbMediaInfo[] = {
  //
  // Systen NvStorage FVB
  //
  {
    NV_STORAGE_FVB_SIZE,
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      NV_STORAGE_FVB_SIZE,
      EFI_FVH_SIGNATURE,
      EFI_FVB2_READ_ENABLED_CAP |
        EFI_FVB2_READ_STATUS |
        EFI_FVB2_WRITE_ENABLED_CAP |
        EFI_FVB2_WRITE_STATUS |
        EFI_FVB2_ERASE_POLARITY,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[1]
      1,  // Revision
      {
        NV_STORAGE_FVB_BLOCK_NUM,
        FV_BLOCK_SIZE,
      }
    },
    {
      0,
      0
    }
  },
  //
  // System FTW FVB
  //
  {
    NV_FTW_FVB_SIZE,
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      NV_FTW_FVB_SIZE,
      EFI_FVH_SIGNATURE,
      EFI_FVB2_READ_ENABLED_CAP |
        EFI_FVB2_READ_STATUS |
        EFI_FVB2_WRITE_ENABLED_CAP |
        EFI_FVB2_WRITE_STATUS |
        EFI_FVB2_ERASE_POLARITY,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[1]
      1,  // Revision
      {
        NV_FTW_FVB_BLOCK_NUM,
        FV_BLOCK_SIZE,
      }
    },
    {
      0,
      0
    }
  }
};

EFI_STATUS
GetFvbInfo (
  IN  UINT64                        FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER    **FvbInfo
  )
{
  UINTN Index;

  for (Index = 0; Index < sizeof (mPlatformFvbMediaInfo) / sizeof (EFI_FVB_MEDIA_INFO); Index += 1) {
    if (mPlatformFvbMediaInfo[Index].FvLength == FvLength) {
      *FvbInfo = &mPlatformFvbMediaInfo[Index].FvbInfo;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
