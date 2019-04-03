/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

--*/

/** @file
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PlatformGopPolicy.h>

#include <Guid/SetupVariable.h>
#include <SetupMode.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

PLATFORM_GOP_POLICY_PROTOCOL  mPlatformGOPPolicy;

//
// Function implementations
//

/**
  The function will execute with as the platform policy, and gives
  the Platform Lid Status. IBV/OEM can customize this code for their specific
  policy action.

  @param CurrentLidStatus  Gives the current LID Status

  @retval EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
GetPlatformLidStatus (
   OUT LID_STATUS *CurrentLidStatus
)
{
  *CurrentLidStatus = LidOpen;

  return EFI_SUCCESS;
}

/**
  The function will execute and gives the Video Bios Table Size and Address.

  @param VbtAddress  Gives the Physical Address of Video BIOS Table

  @param VbtSize     Gives the Size of Video BIOS Table

  @retval EFI_STATUS.

**/

EFI_STATUS
EFIAPI
GetVbtData (
   OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
   OUT UINT32 *VbtSize
)
{
  EFI_STATUS                    Status;
  UINTN                         FvProtocolCount;
  EFI_HANDLE                    *FvHandles;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  UINTN                         Index;
  UINT32                        AuthenticationStatus;

  UINT8                         *Buffer;
  UINTN                         VbtBufferSize;

  Buffer = 0;
  FvHandles       = NULL;

  if (VbtAddress == NULL || VbtSize == NULL){
    return EFI_INVALID_PARAMETER;
  }
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvProtocolCount,
                  &FvHandles
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < FvProtocolCount; Index++) {
      Status = gBS->HandleProtocol (
                      FvHandles[Index],
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **) &Fv
                      );
      VbtBufferSize = 0;
      Status = Fv->ReadSection (
                     Fv,
                     &gBmpImageGuid,
                     EFI_SECTION_RAW,
                     0,
                    (void **)&Buffer,
                     &VbtBufferSize,
                     &AuthenticationStatus
                     );

      if (!EFI_ERROR (Status)) {
        *VbtAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
        *VbtSize = (UINT32)VbtBufferSize;
        Status = EFI_SUCCESS;
        break;
      }
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (FvHandles != NULL) {
    gBS->FreePool (FvHandles);
    FvHandles = NULL;
  }

  return Status;
}

/**
  Entry point for the Platform GOP Policy Driver.

  @param ImageHandle       Image handle of this driver.
  @param SystemTable       Global system service table.

  @retval EFI_SUCCESS           Initialization complete.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.

**/

EFI_STATUS
EFIAPI
PlatformGOPPolicyEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )

{
  EFI_STATUS  Status = EFI_SUCCESS;
  SYSTEM_CONFIGURATION          SystemConfiguration;
  UINTN       VarSize;


  gBS = SystemTable->BootServices;

  gBS->SetMem (
         &mPlatformGOPPolicy,
         sizeof (PLATFORM_GOP_POLICY_PROTOCOL),
         0
         );

  mPlatformGOPPolicy.Revision                = PLATFORM_GOP_POLICY_PROTOCOL_REVISION_01;
  mPlatformGOPPolicy.GetPlatformLidStatus    = GetPlatformLidStatus;
  mPlatformGOPPolicy.GetVbtData              = GetVbtData;

  //
  // Install protocol to allow access to this Policy.
  //
  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }
  
  if (SystemConfiguration.GOPEnable == 1)
  {
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gPlatformGOPPolicyGuid,
                  &mPlatformGOPPolicy,
                  NULL
                  );
  }

  return Status;
}
