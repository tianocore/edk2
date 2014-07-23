/** @file
  Read EDID information and parse EDID information.

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CirrusLogic5430.h"
#include "CirrusLogic5430I2c.h"

//
// EDID block
//
typedef struct {
  UINT8   Header[8];                        //EDID header "00 FF FF FF FF FF FF 00"
  UINT16  ManufactureName;                  //EISA 3-character ID
  UINT16  ProductCode;                      //Vendor assigned code
  UINT32  SerialNumber;                     //32-bit serial number
  UINT8   WeekOfManufacture;                //Week number
  UINT8   YearOfManufacture;                //Year
  UINT8   EdidVersion;                      //EDID Structure Version
  UINT8   EdidRevision;                     //EDID Structure Revision
  UINT8   VideoInputDefinition;
  UINT8   MaxHorizontalImageSize;           //cm
  UINT8   MaxVerticalImageSize;             //cm
  UINT8   DisplayTransferCharacteristic;
  UINT8   FeatureSupport;
  UINT8   RedGreenLowBits;                  //Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
  UINT8   BlueWhiteLowBits;                 //Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
  UINT8   RedX;                             //Red-x Bits 9 - 2
  UINT8   RedY;                             //Red-y Bits 9 - 2
  UINT8   GreenX;                           //Green-x Bits 9 - 2
  UINT8   GreenY;                           //Green-y Bits 9 - 2
  UINT8   BlueX;                            //Blue-x Bits 9 - 2
  UINT8   BlueY;                            //Blue-y Bits 9 - 2
  UINT8   WhiteX;                           //White-x Bits 9 - 2
  UINT8   WhiteY;                           //White-x Bits 9 - 2
  UINT8   EstablishedTimings[3];
  UINT8   StandardTimingIdentification[16];
  UINT8   DetailedTimingDescriptions[72];
  UINT8   ExtensionFlag;                    //Number of (optional) 128-byte EDID extension blocks to follow
  UINT8   Checksum;
} EDID_BLOCK;

#define EDID_BLOCK_SIZE                        128
#define VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER 17

typedef struct {
  UINT16  HorizontalResolution;
  UINT16  VerticalResolution;
  UINT16  RefreshRate;
} EDID_TIMING;

typedef struct {
  UINT32  ValidNumber;
  UINT32  Key[VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER];
} VALID_EDID_TIMING;

//
// Standard timing defined by VESA EDID
//
EDID_TIMING mVbeEstablishedEdidTiming[] = {
  //
  // Established Timing I
  //
  {800, 600, 60},
  {800, 600, 56},
  {640, 480, 75},
  {640, 480, 72},
  {640, 480, 67},
  {640, 480, 60},
  {720, 400, 88},
  {720, 400, 70},
  //
  // Established Timing II
  //
  {1280, 1024, 75},
  {1024,  768, 75},
  {1024,  768, 70},
  {1024,  768, 60},
  {1024,  768, 87},
  {832,   624, 75},
  {800,   600, 75},
  {800,   600, 72},
  //
  // Established Timing III
  //
  {1152, 870, 75}
};

/**
  Read EDID information from I2C Bus on CirrusLogic.

  @param  Private             Pointer to CIRRUS_LOGIC_5430_PRIVATE_DATA.
  @param  EdidDataBlock       Pointer to EDID data block.
  @param  EdidSize            Returned EDID block size.

  @retval EFI_UNSUPPORTED
  @retval EFI_SUCCESS

**/
EFI_STATUS
ReadEdidData (
  CIRRUS_LOGIC_5430_PRIVATE_DATA     *Private,
  UINT8                              **EdidDataBlock,
  UINTN                              *EdidSize
  )
{
  UINTN             Index;
  UINT8             EdidData[EDID_BLOCK_SIZE * 2];
  UINT8             *ValidEdid;
  UINT64            Signature;

  for (Index = 0; Index < EDID_BLOCK_SIZE * 2; Index ++) {
    I2cReadByte (Private->PciIo, 0xa0, (UINT8)Index, &EdidData[Index]);
  }

  //
  // Search for the EDID signature
  //
  ValidEdid = &EdidData[0];
  Signature = 0x00ffffffffffff00ull;
  for (Index = 0; Index < EDID_BLOCK_SIZE * 2; Index ++, ValidEdid ++) {
    if (CompareMem (ValidEdid, &Signature, 8) == 0) {
      break;
    }
  }

  if (Index == 256) {
    //
    // No EDID signature found
    //
    return EFI_UNSUPPORTED;
  }

  *EdidDataBlock = AllocateCopyPool (
                     EDID_BLOCK_SIZE,
                     ValidEdid
                     );
  if (*EdidDataBlock == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Currently only support EDID 1.x
  //
  *EdidSize = EDID_BLOCK_SIZE;

  return EFI_SUCCESS;
}

/**
  Generate a search key for a specified timing data.

  @param  EdidTiming             Pointer to EDID timing

  @return The 32 bit unique key for search.

**/
UINT32
CalculateEdidKey (
  EDID_TIMING       *EdidTiming
  )
{
  UINT32 Key;

  //
  // Be sure no conflicts for all standard timing defined by VESA.
  //
  Key = (EdidTiming->HorizontalResolution * 2) + EdidTiming->VerticalResolution;
  return Key;
}

/**
  Search a specified Timing in all the valid EDID timings.

  @param  ValidEdidTiming        All valid EDID timing information.
  @param  EdidTiming             The Timing to search for.

  @retval TRUE                   Found.
  @retval FALSE                  Not found.

**/
BOOLEAN
SearchEdidTiming (
  VALID_EDID_TIMING *ValidEdidTiming,
  EDID_TIMING       *EdidTiming
  )
{
  UINT32 Index;
  UINT32 Key;

  Key = CalculateEdidKey (EdidTiming);

  for (Index = 0; Index < ValidEdidTiming->ValidNumber; Index ++) {
    if (Key == ValidEdidTiming->Key[Index]) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Parse the Established Timing and Standard Timing in EDID data block.

  @param  EdidBuffer             Pointer to EDID data block
  @param  ValidEdidTiming        Valid EDID timing information

  @retval TRUE                   The EDID data is valid.
  @retval FALSE                  The EDID data is invalid.

**/
BOOLEAN
ParseEdidData (
  UINT8                         *EdidBuffer,
  VALID_EDID_TIMING             *ValidEdidTiming
  )
{
  UINT8        CheckSum;
  UINT32       Index;
  UINT32       ValidNumber;
  UINT32       TimingBits;
  UINT8        *BufferIndex;
  UINT16       HorizontalResolution;
  UINT16       VerticalResolution;
  UINT8        AspectRatio;
  UINT8        RefreshRate;
  EDID_TIMING  TempTiming;
  EDID_BLOCK   *EdidDataBlock;

  EdidDataBlock = (EDID_BLOCK *) EdidBuffer;

  //
  // Check the checksum of EDID data
  //
  CheckSum = 0;
  for (Index = 0; Index < EDID_BLOCK_SIZE; Index ++) {
    CheckSum = (UINT8) (CheckSum + EdidBuffer[Index]);
  }
  if (CheckSum != 0) {
    return FALSE;
  }

  ValidNumber = 0;
  SetMem (ValidEdidTiming, sizeof (VALID_EDID_TIMING), 0);

  if ((EdidDataBlock->EstablishedTimings[0] != 0) ||
      (EdidDataBlock->EstablishedTimings[1] != 0) ||
      (EdidDataBlock->EstablishedTimings[2] != 0)
      ) {
    //
    // Established timing data
    //
    TimingBits = EdidDataBlock->EstablishedTimings[0] |
                 (EdidDataBlock->EstablishedTimings[1] << 8) |
                 ((EdidDataBlock->EstablishedTimings[2] & 0x80) << 9) ;
    for (Index = 0; Index < VBE_EDID_ESTABLISHED_TIMING_MAX_NUMBER; Index ++) {
      if (TimingBits & 0x1) {
        ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey (&mVbeEstablishedEdidTiming[Index]);
        ValidNumber ++;
      }
      TimingBits = TimingBits >> 1;
    }
  } else {
    //
    // If no Established timing data, read the standard timing data
    //
    BufferIndex = &EdidDataBlock->StandardTimingIdentification[0];
    for (Index = 0; Index < 8; Index ++) {
      if ((BufferIndex[0] != 0x1) && (BufferIndex[1] != 0x1)){
        //
        // A valid Standard Timing
        //
        HorizontalResolution = (UINT16) (BufferIndex[0] * 8 + 248);
        AspectRatio = (UINT8) (BufferIndex[1] >> 6);
        switch (AspectRatio) {
          case 0:
            VerticalResolution = (UINT16) (HorizontalResolution / 16 * 10);
            break;
          case 1:
            VerticalResolution = (UINT16) (HorizontalResolution / 4 * 3);
            break;
          case 2:
            VerticalResolution = (UINT16) (HorizontalResolution / 5 * 4);
            break;
          case 3:
            VerticalResolution = (UINT16) (HorizontalResolution / 16 * 9);
            break;
          default:
            VerticalResolution = (UINT16) (HorizontalResolution / 4 * 3);
            break;
        }
        RefreshRate = (UINT8) ((BufferIndex[1] & 0x1f) + 60);
        TempTiming.HorizontalResolution = HorizontalResolution;
        TempTiming.VerticalResolution = VerticalResolution;
        TempTiming.RefreshRate = RefreshRate;
        ValidEdidTiming->Key[ValidNumber] = CalculateEdidKey (&TempTiming);
        ValidNumber ++;
      }
      BufferIndex += 2;
    }
  }

  ValidEdidTiming->ValidNumber = ValidNumber;
  return TRUE;
}

/**
  Construct the valid video modes for CirrusLogic5430.

**/
EFI_STATUS
CirrusLogic5430VideoModeSetup (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                             Status;
  UINT32                                 Index;
  BOOLEAN                                EdidFound;
  EFI_EDID_OVERRIDE_PROTOCOL             *EdidOverride;
  UINT32                                 EdidAttributes;
  BOOLEAN                                EdidOverrideFound;
  UINTN                                  EdidOverrideDataSize;
  UINT8                                  *EdidOverrideDataBlock;
  UINTN                                  EdidDiscoveredDataSize;
  UINT8                                  *EdidDiscoveredDataBlock;
  UINTN                                  EdidActiveDataSize;
  UINT8                                  *EdidActiveDataBlock;
  VALID_EDID_TIMING                      ValidEdidTiming;
  UINT32                                 ValidModeCount;
  CIRRUS_LOGIC_5430_MODE_DATA            *ModeData;
  BOOLEAN                                TimingMatch;
  CIRRUS_LOGIC_5430_VIDEO_MODES          *VideoMode;
  EDID_TIMING                            TempTiming;

  //
  // setup EDID information
  //
  Private->EdidDiscovered.Edid       = NULL;
  Private->EdidDiscovered.SizeOfEdid = 0;
  Private->EdidActive.Edid           = NULL;
  Private->EdidActive.SizeOfEdid     = 0;

  EdidFound               = FALSE;
  EdidOverrideFound       = FALSE;
  EdidAttributes          = 0xff;
  EdidOverrideDataSize    = 0;
  EdidOverrideDataBlock   = NULL;
  EdidActiveDataSize      = 0;
  EdidActiveDataBlock     = NULL;
  EdidDiscoveredDataBlock = NULL;

  //
  // Find EDID Override protocol firstly, this protocol is installed by platform if needed.
  //
  Status = gBS->LocateProtocol (
                   &gEfiEdidOverrideProtocolGuid,
                   NULL,
                   (VOID **) &EdidOverride
                   );
  if (!EFI_ERROR (Status)) {
    //
    // Allocate double size of VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE to avoid overflow
    //
    EdidOverrideDataBlock = AllocatePool (EDID_BLOCK_SIZE * 2);
    if (NULL == EdidOverrideDataBlock) {
  		Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = EdidOverride->GetEdid (
                             EdidOverride,
                             Private->Handle,
                             &EdidAttributes,
                             &EdidOverrideDataSize,
                             (UINT8 **) &EdidOverrideDataBlock
                             );
    if (!EFI_ERROR (Status)  &&
         EdidAttributes == 0 &&
         EdidOverrideDataSize != 0) {
      //
      // Succeeded to get EDID Override Data
      //
      EdidOverrideFound = TRUE;
    }
  }

  if (EdidOverrideFound != TRUE || EdidAttributes == EFI_EDID_OVERRIDE_DONT_OVERRIDE) {
    //
    // If EDID Override data doesn't exist or EFI_EDID_OVERRIDE_DONT_OVERRIDE returned,
    // read EDID information through I2C Bus
    //
    if (ReadEdidData (Private, &EdidDiscoveredDataBlock, &EdidDiscoveredDataSize) == EFI_SUCCESS) {
      Private->EdidDiscovered.SizeOfEdid = (UINT32) EdidDiscoveredDataSize;
     	Private->EdidDiscovered.Edid = (UINT8 *) AllocateCopyPool (
                                                          EdidDiscoveredDataSize,
                                                          EdidDiscoveredDataBlock
     																										  );

      if (NULL == Private->EdidDiscovered.Edid) {
     	  Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      EdidActiveDataSize  = Private->EdidDiscovered.SizeOfEdid;
      EdidActiveDataBlock = Private->EdidDiscovered.Edid;

      EdidFound = TRUE;
    }
  }

  if (EdidFound != TRUE && EdidOverrideFound == TRUE) {
    EdidActiveDataSize  = EdidOverrideDataSize;
    EdidActiveDataBlock = EdidOverrideDataBlock;
    EdidFound = TRUE;
 	}

 	if (EdidFound == TRUE) {
    //
    // Parse EDID data structure to retrieve modes supported by monitor
    //
    if (ParseEdidData ((UINT8 *) EdidActiveDataBlock, &ValidEdidTiming) == TRUE) {
      //
      // Copy EDID Override Data to EDID Active Data
      //
      Private->EdidActive.SizeOfEdid = (UINT32) EdidActiveDataSize;
      Private->EdidActive.Edid = (UINT8 *) AllocateCopyPool (
                                             EdidActiveDataSize,
                                             EdidActiveDataBlock
                                             );
      if (NULL == Private->EdidActive.Edid) {
   		  Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
  } else {
    Private->EdidActive.SizeOfEdid = 0;
    Private->EdidActive.Edid = NULL;
    EdidFound = FALSE;
  }

  if (EdidFound) {
    //
    // Initialize the private mode data with the supported modes.
    //
    ValidModeCount = 0;
    ModeData = &Private->ModeData[0];
    VideoMode = &CirrusLogic5430VideoModes[0];
    for (Index = 0; Index < CIRRUS_LOGIC_5430_MODE_COUNT; Index++) {

      TimingMatch = TRUE;

      //
      // Check whether match with CirrusLogic5430 video mode
      //
      TempTiming.HorizontalResolution = (UINT16) VideoMode->Width;
      TempTiming.VerticalResolution   = (UINT16) VideoMode->Height;
      TempTiming.RefreshRate          = (UINT16) VideoMode->RefreshRate;
      if (SearchEdidTiming (&ValidEdidTiming, &TempTiming) != TRUE) {
        TimingMatch = FALSE;
      }

      //
      // Not export Mode 0x0 as GOP mode, this is not defined in spec.
      //
      if ((VideoMode->Width == 0) || (VideoMode->Height == 0)) {
        TimingMatch = FALSE;
      }

      if (TimingMatch) {
        ModeData->ModeNumber = Index;
        ModeData->HorizontalResolution          = VideoMode->Width;
        ModeData->VerticalResolution            = VideoMode->Height;
        ModeData->ColorDepth                    = VideoMode->ColorDepth;
        ModeData->RefreshRate                   = VideoMode->RefreshRate;

        ModeData ++;
        ValidModeCount ++;
      }

      VideoMode ++;
    }

    Private->MaxMode = ValidModeCount;

  } else {
    //
    // If EDID information wasn't found
    //
    ModeData = &Private->ModeData[0];
    VideoMode = &CirrusLogic5430VideoModes[0];
    for (Index = 0; Index < CIRRUS_LOGIC_5430_MODE_COUNT; Index ++) {
      ModeData->ModeNumber = Index;
      ModeData->HorizontalResolution          = VideoMode->Width;
      ModeData->VerticalResolution            = VideoMode->Height;
      ModeData->ColorDepth                    = VideoMode->ColorDepth;
      ModeData->RefreshRate                   = VideoMode->RefreshRate;

      ModeData ++ ;
      VideoMode ++;
    }
    Private->MaxMode = CIRRUS_LOGIC_5430_MODE_COUNT;
  }

  if (EdidOverrideDataBlock != NULL) {
    FreePool (EdidOverrideDataBlock);
  }

  return EFI_SUCCESS;

Done:
  if (EdidOverrideDataBlock != NULL) {
    FreePool (EdidOverrideDataBlock);
  }
  if (Private->EdidDiscovered.Edid != NULL) {
    FreePool (Private->EdidDiscovered.Edid);
  }
  if (Private->EdidDiscovered.Edid != NULL) {
    FreePool (Private->EdidActive.Edid);
  }

  return EFI_DEVICE_ERROR;
}
