/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php.                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiSmm.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Protocol/SmmCommunication.h>

#include <Guid/MemoryProfile.h>

EFI_SMM_COMMUNICATION_PROTOCOL *mSmmCommunication = NULL;

/**
  Get the GUID file name from the file path.

  @param FilePath  File path.

  @return The GUID file name from the file path.

**/
EFI_GUID *
GetFileNameFromFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *ThisFilePath;

  ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) FilePath;
  while (!IsDevicePathEnd (ThisFilePath)) {
    if ((DevicePathType (ThisFilePath) == MEDIA_DEVICE_PATH) && (DevicePathSubType (ThisFilePath) == MEDIA_PIWG_FW_FILE_DP)) {
      return &ThisFilePath->FvFileName;
    }
    ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) NextDevicePathNode (ThisFilePath);
  }

  return NULL;
}

/**
  Register SMM image to SMRAM profile.

  @param[in] FilePath           File path of the image.
  @param[in] ImageBuffer        Image base address.
  @param[in] NumberOfPage       Number of page.

  @retval TRUE                  Register success.
  @retval FALSE                 Register fail.

**/
BOOLEAN
RegisterSmramProfileImage (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN PHYSICAL_ADDRESS           ImageBuffer,
  IN UINTN                      NumberOfPage
  )
{
  EFI_GUID                                      *FileName;
  EFI_STATUS                                    Status;
  UINTN                                         CommSize;
  UINT8                                         CommBuffer[sizeof (EFI_GUID) + sizeof (UINTN) + sizeof (SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE)];
  EFI_SMM_COMMUNICATE_HEADER                    *CommHeader;
  SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE        *CommRegisterImage;

  if ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT1) == 0) {
    return FALSE;
  }

  FileName = GetFileNameFromFilePath (FilePath);

  if (mSmmCommunication == NULL) {
    Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &mSmmCommunication);
    ASSERT_EFI_ERROR (Status);
  }

  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof (gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE);

  CommRegisterImage = (SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommRegisterImage->Header.Command      = SMRAM_PROFILE_COMMAND_REGISTER_IMAGE;
  CommRegisterImage->Header.DataLength   = sizeof (*CommRegisterImage);
  CommRegisterImage->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&CommRegisterImage->FileName, FileName, sizeof(EFI_GUID));
  CommRegisterImage->ImageBuffer         = ImageBuffer;
  CommRegisterImage->NumberOfPage        = NumberOfPage;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Status = mSmmCommunication->Communicate (mSmmCommunication, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  if (CommRegisterImage->Header.ReturnStatus != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Unregister SMM image from SMRAM profile.

  @param[in] FilePath           File path of the image.
  @param[in] ImageBuffer        Image base address.
  @param[in] NumberOfPage       Number of page.

  @retval TRUE                  Unregister success.
  @retval FALSE                 Unregister fail.

**/
BOOLEAN
UnregisterSmramProfileImage (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN PHYSICAL_ADDRESS           ImageBuffer,
  IN UINTN                      NumberOfPage
  )
{
  EFI_GUID                                      *FileName;
  EFI_STATUS                                    Status;
  UINTN                                         CommSize;
  UINT8                                         CommBuffer[sizeof (EFI_GUID) + sizeof (UINTN) + sizeof (SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE)];
  EFI_SMM_COMMUNICATE_HEADER                    *CommHeader;
  SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE      *CommUnregisterImage;

  if ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT1) == 0) {
    return FALSE;
  }

  FileName = GetFileNameFromFilePath (FilePath);

  if (mSmmCommunication == NULL) {
    Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &mSmmCommunication);
    ASSERT_EFI_ERROR (Status);
  }

  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *)&CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof(gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof(SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE);

  CommUnregisterImage = (SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE *)&CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommUnregisterImage->Header.Command      = SMRAM_PROFILE_COMMAND_UNREGISTER_IMAGE;
  CommUnregisterImage->Header.DataLength   = sizeof (*CommUnregisterImage);
  CommUnregisterImage->Header.ReturnStatus = (UINT64)-1;
  CopyMem (&CommUnregisterImage->FileName, FileName, sizeof(EFI_GUID));
  CommUnregisterImage->ImageBuffer         = ImageBuffer;
  CommUnregisterImage->NumberOfPage        = NumberOfPage;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Status = mSmmCommunication->Communicate (mSmmCommunication, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  if (CommUnregisterImage->Header.ReturnStatus != 0) {
    return FALSE;
  }

  return TRUE;
}
