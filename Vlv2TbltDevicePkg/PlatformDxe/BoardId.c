/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  BoardId.c

Abstract:

  Initialization for the board ID.

  This code should be common across a chipset family of products.



--*/

#include "PchRegs.h"
#include "PlatformDxe.h"
#include <Guid/IdccData.h>
#include <Guid/EfiVpdData.h>
#include <Protocol/DataHub.h>


extern EFI_GUID mPlatformDriverGuid;

//
// Global module data
//
UINT32 mBoardId;
UINT8  mBoardIdIndex;
EFI_BOARD_FEATURES mBoardFeatures;
UINT16 mSubsystemDeviceId;
UINT16 mSubsystemAudioDeviceId;
CHAR8  BoardAaNumber[7];
BOOLEAN mFoundAANum;

/**

  Write the boardid variable if it does not already exist.

**/
VOID
InitializeBoardId (
  )
{

  UINT32                        BoardIdBufferSize;
  EFI_IDCC_BOARD_FORM_FACTOR    IdccBoardFormFactor;
  EFI_DATA_HUB_PROTOCOL         *DataHub;
  EFI_STATUS                    Status;
  DMI_DATA                      DmiDataVariable;
  UINTN                         Size;
#if defined(DUPLICATE_AA_NO_BASE_ADDR)
  CHAR8                         DuplicateAaNoAscii[sizeof(DmiDataVariable.BaseBoardVersion)];
  UINTN                         iter;
#endif
#if defined(GPIO_BOARD_ID_SUPPORT) && GPIO_BOARD_ID_SUPPORT != 0
  UINT8                         Data8;
#endif

  //
  // Update data from the updatable DMI data area
  //
  Size = sizeof (DMI_DATA);
  SetMem(&DmiDataVariable, Size, 0xFF);
  Status = gRT->GetVariable (
                  DMI_DATA_NAME,
                  &gDmiDataGuid,
                  NULL,
                  &Size,
                  &DmiDataVariable
                  );

#if defined(DUPLICATE_AA_NO_BASE_ADDR)
  //
  // Get AA# from flash descriptor region
  //
  EfiSetMem(DuplicateAaNoAscii, sizeof(DuplicateAaNoAscii), 0xFF);
  FlashRead((UINT8 *)(UINTN)DUPLICATE_AA_NO_BASE_ADDR,
            (UINT8 *)DuplicateAaNoAscii,
            sizeof(DuplicateAaNoAscii));

  //
  // Validate AA# read from VPD
  //
  for (iter = 0; iter < sizeof(DuplicateAaNoAscii); iter++) {
     if ((DuplicateAaNoAscii[iter] != 0xFF) &&
         (DuplicateAaNoAscii[iter] != DmiDataVariable.BaseBoardVersion[iter])) {
       DmiDataVariable.BaseBoardVersion[iter] = DuplicateAaNoAscii[iter];
     }
  }

  Status = EFI_SUCCESS;
#endif

  mFoundAANum = FALSE;

  //
  // No variable...no copy
  //
  if (EFI_ERROR (Status)) {
    mBoardIdIndex = 0; // If we can't find the BoardId in the table, use the first entry
  } else {
  	//
    // This is the correct method of checking for AA#.
    //
    CopyMem(&BoardAaNumber, ((((UINT8*)&DmiDataVariable.BaseBoardVersion)+2)), 6);
    BoardAaNumber[6] = 0;
    for (mBoardIdIndex = 0; mBoardIdIndex < mBoardIdDecodeTableSize; mBoardIdIndex++) {
      if (AsciiStrnCmp(mBoardIdDecodeTable[mBoardIdIndex].AaNumber, BoardAaNumber, 6) == 0) {
        mFoundAANum = TRUE;
        break;
      }
    }

    if(!mFoundAANum) {
    	//
      // Add check for AA#'s that is programmed without the AA as leading chars.
      //
      CopyMem(&BoardAaNumber, (((UINT8*)&DmiDataVariable.BaseBoardVersion)), 6);
      BoardAaNumber[6] = 0;
      for (mBoardIdIndex = 0; mBoardIdIndex < mBoardIdDecodeTableSize; mBoardIdIndex++) {
        if (AsciiStrnCmp(mBoardIdDecodeTable[mBoardIdIndex].AaNumber, BoardAaNumber, 6) == 0) {
          mFoundAANum = TRUE;
          break;
        }
      }
    }
  }

#if defined(GPIO_BOARD_ID_SUPPORT) && GPIO_BOARD_ID_SUPPORT != 0
  //
  // If we can't find the BoardAA# in the table, find BoardId
  //
  if (mFoundAANum != TRUE) {
    //
    // BoardID BIT    Location
    //  0             GPIO33  (ICH)
    //  1             GPIO34  (ICH)
    //
    Data8 = IoRead8(GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL2);

    //
    // BoardId[0]
    //
    mBoardId = (UINT32)((Data8 >> 1) & BIT0);
    //
    // BoardId[1]
    //
    mBoardId |= (UINT32)((Data8 >> 1) & BIT1);

    for (mBoardIdIndex = 0; mBoardIdIndex < mBoardIdDecodeTableSize; mBoardIdIndex++) {
      if (mBoardIdDecodeTable[mBoardIdIndex].BoardId == mBoardId) {
        break;
      }
    }
#endif
    if (mBoardIdIndex == mBoardIdDecodeTableSize) {
      mBoardIdIndex = 0; // If we can't find the BoardId in the table, use the first entry
    }
#if defined(GPIO_BOARD_ID_SUPPORT) && GPIO_BOARD_ID_SUPPORT != 0
  }
#endif

  mBoardFeatures = mBoardIdDecodeTable[mBoardIdIndex].Features;
  mSubsystemDeviceId = mBoardIdDecodeTable[mBoardIdIndex].SubsystemDeviceId;
  mSubsystemAudioDeviceId = mBoardIdDecodeTable[mBoardIdIndex].AudioSubsystemDeviceId;

  //
  // Set the BoardFeatures variable
  //
  BoardIdBufferSize = sizeof (mBoardFeatures);
  gRT->SetVariable (
         BOARD_FEATURES_NAME,
         &gEfiBoardFeaturesGuid,
         EFI_VARIABLE_NON_VOLATILE |
         EFI_VARIABLE_BOOTSERVICE_ACCESS |
         EFI_VARIABLE_RUNTIME_ACCESS,
         BoardIdBufferSize,
         &mBoardFeatures
         );

  //
  // Get the Data Hub protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  (VOID **) &DataHub
                  );
  if (!(EFI_ERROR(Status))) {
    //
    // Fill out data
    //
    IdccBoardFormFactor.IdccHeader.Type = EFI_IDCC_BOARD_FORM_FACTOR_TYPE;
    IdccBoardFormFactor.IdccHeader.RecordLength = sizeof(EFI_IDCC_BOARD_FORM_FACTOR);
    if ((mBoardFeatures & B_BOARD_FEATURES_FORM_FACTOR_ATX) || (mBoardFeatures & B_BOARD_FEATURES_FORM_FACTOR_MICRO_ATX)) {
        IdccBoardFormFactor.BoardFormFactor = ATX_FORM_FACTOR; // ATX
    } else {
        IdccBoardFormFactor.BoardFormFactor = BTX_FORM_FACTOR; // BTX
    }

    //
    // Publish the Board Form Factor value for IDCC
    //
    Status = DataHub->LogData (
                        DataHub,
                        &gIdccDataHubGuid,
                        &mPlatformDriverGuid,
                        EFI_DATA_RECORD_CLASS_DATA,
                        &IdccBoardFormFactor,
                        sizeof(EFI_IDCC_BOARD_FORM_FACTOR)
                        );
  }
}

