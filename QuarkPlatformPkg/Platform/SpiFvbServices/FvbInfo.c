/** @file
Defines data structure that is the volume header found.These data is intent
to decouple FVB driver with FV header.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include <PiDxe.h>
#include "FwBlockService.h"


//#define FVB_MEDIA_BLOCK_SIZE  PcdGet32(PcdFlashMinEraseSize)
#define FVB_MEDIA_BLOCK_SIZE  0x1000

typedef struct {
  EFI_PHYSICAL_ADDRESS        BaseAddress;
  EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
  //
  //EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
  //
  EFI_FV_BLOCK_MAP_ENTRY      End[1];
} EFI_FVB2_MEDIA_INFO;

//
// This data structure contains a template of all correct FV headers, which is used to restore
// Fv header if it's corrupted.
//
EFI_FVB2_MEDIA_INFO mPlatformFvbMediaInfo[] = {
  //
  // Main BIOS FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum, check the FD for the value.
      0,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  },
  //
  // Systen NvStorage FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum which will be calucated dynamically.
      0,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  },
  //
  // Recovery BIOS FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum which will be calucated dynamically.
      0,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  },
  //
  // Payload FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum which will be calucated dynamically.
      0x60,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  }
};


//
// FTW working space and FTW spare space don't have FV header.
// We need create one for them and use it for FVB protocol.
//
EFI_FVB2_MEDIA_INFO mPlatformFtwFvbInfo[] = {
  //
  // System variable FTW working FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum which will be calucated dynamically.
      0,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  },
  //
  // Systen NV variable FTW spare FVB
  //
  {
    0,
    {
      {0,}, //ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      0,
      EFI_FVH_SIGNATURE,
      0x0004feff, // check MdePkg/Include/Pi/PiFirmwareVolume.h for details on EFI_FVB_ATTRIBUTES_2
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,    //CheckSum which will be calucated dynamically.
      0,    //ExtHeaderOffset
      {0,}, //Reserved[1]
      2,    //Revision
      {
        {
          0,
          0,
        }
      }
    },
    {
      {
        0,
        0
      }
    }
  }
};



EFI_STATUS
GetFtwFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS         FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER   **FvbInfo
  )
{
  UINTN                       Index;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;

  //
  // Init Fvb data
  //
  mPlatformFtwFvbInfo[0].BaseAddress      = PcdGet32 (PcdFlashNvStorageFtwWorkingBase);
  mPlatformFtwFvbInfo[0].FvbInfo.FvLength = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  mPlatformFtwFvbInfo[0].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashNvStorageFtwWorkingSize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFtwFvbInfo[0].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashNvStorageFtwWorkingSize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  mPlatformFtwFvbInfo[1].BaseAddress      = PcdGet32 (PcdFlashNvStorageFtwSpareBase);
  mPlatformFtwFvbInfo[1].FvbInfo.FvLength = PcdGet32 (PcdFlashNvStorageFtwSpareSize);
  mPlatformFtwFvbInfo[1].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashNvStorageFtwSpareSize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFtwFvbInfo[1].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashNvStorageFtwSpareSize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  for (Index=0; Index < sizeof (mPlatformFtwFvbInfo)/sizeof (mPlatformFtwFvbInfo[0]); Index += 1) {
    if (mPlatformFtwFvbInfo[Index].BaseAddress == FvBaseAddress) {
      FvHeader =  &mPlatformFtwFvbInfo[Index].FvbInfo;
      //
      // Update the checksum value of FV header.
      //
      FvHeader->Checksum = CalculateCheckSum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

      *FvbInfo = FvHeader;

      DEBUG ((EFI_D_INFO, "\nFTW BaseAddr: 0x%lx \n", FvBaseAddress));
      DEBUG ((EFI_D_INFO, "FvLength: 0x%lx \n", (*FvbInfo)->FvLength));
      DEBUG ((EFI_D_INFO, "HeaderLength: 0x%x \n", (*FvbInfo)->HeaderLength));
      DEBUG ((EFI_D_INFO, "FvBlockMap[0].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[0].NumBlocks));
      DEBUG ((EFI_D_INFO, "FvBlockMap[0].BlockLength: 0x%x \n", (*FvbInfo)->BlockMap[0].Length));
      DEBUG ((EFI_D_INFO, "FvBlockMap[1].NumBlocks: 0x%x \n",   (*FvbInfo)->BlockMap[1].NumBlocks));
      DEBUG ((EFI_D_INFO, "FvBlockMap[1].BlockLength: 0x%x \n\n", (*FvbInfo)->BlockMap[1].Length));

      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}


EFI_STATUS
GetFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS         FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER   **FvbInfo
  )
{
  UINTN                       Index;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;

  //
  // Init Fvb data
  //
  mPlatformFvbMediaInfo[0].BaseAddress      = PcdGet32 (PcdFlashFvMainBase);
  mPlatformFvbMediaInfo[0].FvbInfo.FvLength = PcdGet32 (PcdFlashFvMainSize);
  mPlatformFvbMediaInfo[0].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashFvMainSize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFvbMediaInfo[0].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashFvMainSize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  mPlatformFvbMediaInfo[1].BaseAddress      = PcdGet32 (PcdFlashNvStorageVariableBase);
  mPlatformFvbMediaInfo[1].FvbInfo.FvLength = PcdGet32 (PcdFlashNvStorageVariableSize);
  mPlatformFvbMediaInfo[1].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashNvStorageVariableSize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFvbMediaInfo[1].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashNvStorageVariableSize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  mPlatformFvbMediaInfo[2].BaseAddress      = PcdGet32 (PcdFlashFvRecoveryBase);
  mPlatformFvbMediaInfo[2].FvbInfo.FvLength = PcdGet32 (PcdFlashFvRecoverySize);
  mPlatformFvbMediaInfo[2].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashFvRecoverySize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFvbMediaInfo[2].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashFvRecoverySize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  mPlatformFvbMediaInfo[3].BaseAddress      = PcdGet32 (PcdFlashFvPayloadBase);
  mPlatformFvbMediaInfo[3].FvbInfo.FvLength = PcdGet32 (PcdFlashFvPayloadSize);
  mPlatformFvbMediaInfo[3].FvbInfo.BlockMap[0].NumBlocks = PcdGet32 (PcdFlashFvPayloadSize) / FVB_MEDIA_BLOCK_SIZE;
  mPlatformFvbMediaInfo[3].FvbInfo.BlockMap[0].Length    = FVB_MEDIA_BLOCK_SIZE;
  ASSERT ((PcdGet32 (PcdFlashFvPayloadSize) % FVB_MEDIA_BLOCK_SIZE) == 0);

  for (Index=0; Index < sizeof (mPlatformFvbMediaInfo)/sizeof (mPlatformFvbMediaInfo[0]); Index += 1) {
    if (mPlatformFvbMediaInfo[Index].BaseAddress == FvBaseAddress) {
      FvHeader =  &mPlatformFvbMediaInfo[Index].FvbInfo;
      //
      // Update the checksum value of FV header.
      //
      FvHeader->Checksum = CalculateCheckSum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

      *FvbInfo = FvHeader;

      DEBUG ((EFI_D_INFO, "\nBaseAddr: 0x%lx \n", FvBaseAddress));
      DEBUG ((EFI_D_INFO, "FvLength: 0x%lx \n", (*FvbInfo)->FvLength));
      DEBUG ((EFI_D_INFO, "HeaderLength: 0x%x \n", (*FvbInfo)->HeaderLength));
      DEBUG ((EFI_D_INFO, "FvBlockMap[0].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[0].NumBlocks));
      DEBUG ((EFI_D_INFO, "FvBlockMap[0].BlockLength: 0x%x \n", (*FvbInfo)->BlockMap[0].Length));
      DEBUG ((EFI_D_INFO, "FvBlockMap[1].NumBlocks: 0x%x \n",   (*FvbInfo)->BlockMap[1].NumBlocks));
      DEBUG ((EFI_D_INFO, "FvBlockMap[1].BlockLength: 0x%x \n\n", (*FvbInfo)->BlockMap[1].Length));

      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}
