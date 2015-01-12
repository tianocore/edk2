/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  Recovery.c

Abstract:

  Tiano PEIM to provide the platform recovery functionality.

--*/

#include "PlatformEarlyInit.h"

#define PEI_FVMAIN_COMPACT_GUID \
  {0x4A538818, 0x5AE0, 0x4eb2, 0xB2, 0xEB, 0x48, 0x8b, 0x23, 0x65, 0x70, 0x22};

EFI_GUID FvMainCompactFileGuid = PEI_FVMAIN_COMPACT_GUID;

//
// Required Service
//
EFI_STATUS
EFIAPI
PlatformRecoveryModule (
  IN CONST EFI_PEI_SERVICES                       **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI          *This
  );

//
// Module globals
//

typedef struct {
  EFI_GUID  CapsuleGuid;
  UINT32    HeaderSize;
  UINT32    Flags;
  UINT32    CapsuleImageSize;
  UINT32    SequenceNumber;
  EFI_GUID  InstanceId;
  UINT32    OffsetToSplitInformation;
  UINT32    OffsetToCapsuleBody;
  UINT32    OffsetToOemDefinedHeader;
  UINT32    OffsetToAuthorInformation;
  UINT32    OffsetToRevisionInformation;
  UINT32    OffsetToShortDescription;
  UINT32    OffsetToLongDescription;
  UINT32    OffsetToApplicableDevices;
} OLD_EFI_CAPSULE_HEADER;


static EFI_PEI_RECOVERY_MODULE_PPI mRecoveryPpi = {
  PlatformRecoveryModule
};

static EFI_PEI_PPI_DESCRIPTOR mRecoveryPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiRecoveryModulePpiGuid,
  &mRecoveryPpi
};

/**
  Provide the functionality of the Recovery Module.

  @param PeiServices  General purpose services available to every PEIM.

  @retval Status      EFI_SUCCESS if the interface could be successfully
                      installed

**/
EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = (*PeiServices)->InstallPpi (
                             PeiServices,
                             &mRecoveryPpiList
                             );

  return Status;
}

/**
  Provide the functionality of the Ea Recovery Module.

  @param PeiServices         General purpose services available to every PEIM.
  @param This                Pointer to PEI_RECOVERY_MODULE_INTERFACE.

  @retval EFI_SUCCESS        If the interface could be successfully
                             installed.
  @retval EFI_UNSUPPORTED    Not supported.

**/
EFI_STATUS
EFIAPI
PlatformRecoveryModule (
  IN CONST EFI_PEI_SERVICES               **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI          *This
  )
{
  EFI_STATUS                            Status;
  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI    *DeviceRecoveryModule;
  UINTN                                 NumberOfImageProviders;
  BOOLEAN                               ProviderAvailable;
  UINTN                                 NumberRecoveryCapsules;
  UINTN                                 RecoveryCapsuleSize;
  EFI_GUID                              DeviceId;
  BOOLEAN                               ImageFound;
  EFI_PHYSICAL_ADDRESS                  Address;
  VOID                                  *Buffer;
  OLD_EFI_CAPSULE_HEADER                *CapsuleHeader;
  EFI_PEI_HOB_POINTERS                  Hob;
  EFI_PEI_HOB_POINTERS                  HobOld;
  EFI_HOB_CAPSULE_VOLUME                *CapsuleHob;
  BOOLEAN                               HobUpdate;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  UINTN                                 Index;
  BOOLEAN                                FoundFvMain;
  BOOLEAN                                FoundCapsule;
  static EFI_GUID mEfiCapsuleHeaderGuid = EFI_CAPSULE_GUID;
  EFI_PEI_STALL_PPI                      *StallPpi;

  (*PeiServices)->ReportStatusCode (
                    PeiServices,
                    EFI_PROGRESS_CODE,
                    EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_RECOVERY_BEGIN,
                    0,
                    NULL,
                    NULL
                    );

  Status = (**PeiServices).LocatePpi (
              PeiServices,
              &gEfiPeiStallPpiGuid,
              0,
              NULL,
              &StallPpi
              );
  ASSERT_EFI_ERROR (Status);

  StallPpi->Stall(
              PeiServices,
              StallPpi,
              5000000
              );


  Index = 0;

  Status                  = EFI_SUCCESS;
  HobUpdate               = FALSE;

  ProviderAvailable       = TRUE;
  ImageFound              = FALSE;
  NumberOfImageProviders  = 0;

  DeviceRecoveryModule    = NULL;

  FoundCapsule = FALSE;
  FoundFvMain = FALSE;

  DEBUG ((EFI_D_ERROR | EFI_D_LOAD, "Recovery Entry\n"));

  //
  // Search the platform for some recovery capsule if the DXE IPL
  // discovered a recovery condition and has requested a load.
  //
  while (ProviderAvailable == TRUE) {

    Status = (*PeiServices)->LocatePpi (
                               PeiServices,
                               &gEfiPeiDeviceRecoveryModulePpiGuid,
                               Index,
                               NULL,
                               &DeviceRecoveryModule
                               );

    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Device Recovery PPI located\n"));
      NumberOfImageProviders++;

      Status = DeviceRecoveryModule->GetNumberRecoveryCapsules (
                                       (EFI_PEI_SERVICES**)PeiServices,
                                       DeviceRecoveryModule,
                                       &NumberRecoveryCapsules
                                       );

      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Number Of Recovery Capsules: %d\n", NumberRecoveryCapsules));

      if (NumberRecoveryCapsules == 0) {
        Index++;
      } else {
        break;
      }
    } else {
      ProviderAvailable = FALSE;
    }
  }

  //
  // If there is an image provider, get the capsule ID
  //
  if (ProviderAvailable) {
    RecoveryCapsuleSize = 0;

    Status = DeviceRecoveryModule->GetRecoveryCapsuleInfo (
                                    (EFI_PEI_SERVICES**)PeiServices,
                                    DeviceRecoveryModule,
                                    0,
                                    &RecoveryCapsuleSize,
                                    &DeviceId
                                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Recovery Capsule Size: %d\n", RecoveryCapsuleSize));

    //
    // Only support the 2 capsule types known
    // Future enhancement is to rank-order the selection
    //
    if ((!CompareGuid (&DeviceId, &gRecoveryOnFatIdeDiskGuid)) &&
        (!CompareGuid (&DeviceId, &gRecoveryOnFatFloppyDiskGuid)) &&
        (!CompareGuid (&DeviceId, &gRecoveryOnDataCdGuid)) &&
       (!CompareGuid (&DeviceId, &gRecoveryOnFatUsbDiskGuid))
        ) {
      return EFI_UNSUPPORTED;
    }

    Buffer  = NULL;
    Status = (*PeiServices)->AllocatePages (
                               PeiServices,
                               EfiBootServicesCode,
                               (RecoveryCapsuleSize - 1) / 0x1000 + 1,
                               &Address
                               );

    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "AllocatePage Returns: %r\n", Status));

    if (EFI_ERROR(Status)) {
      return Status;
    }

    Buffer = (UINT8 *) (UINTN) Address;

    (*PeiServices)->ReportStatusCode (
                      PeiServices,
                      EFI_PROGRESS_CODE,
                      EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_CAPSULE_LOAD,
                      0,
                      NULL,
                      NULL
                      );

    Status = DeviceRecoveryModule->LoadRecoveryCapsule (
                                     (EFI_PEI_SERVICES**)PeiServices,
                                     DeviceRecoveryModule,
                                     0,
                                     Buffer
                                     );

    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "LoadRecoveryCapsule Returns: %r\n", Status));

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Update FV Hob if found
    //
    Status = (*PeiServices)->GetHobList (PeiServices, &Hob.Raw);
    HobOld.Raw  = Hob.Raw;
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_FV) {
        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Hob FV Length: %x\n", Hob.FirmwareVolume->Length));
        //
        // BUGBUG Why is it a FV hob if it is greater than 0x50000?
        //
        if (Hob.FirmwareVolume->Length > 0x50000) {
          HobUpdate = TRUE;
          //
          // This looks like the Hob we are interested in
          //
          DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Hob Updated\n"));
          Hob.FirmwareVolume->BaseAddress = (UINTN) Buffer;
          Hob.FirmwareVolume->Length      = RecoveryCapsuleSize;
        }
      }
      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Buffer;
    CapsuleHeader = (OLD_EFI_CAPSULE_HEADER *)Buffer;

    //
    // Check if top of file is a capsule
    //
    if (CompareGuid ((EFI_GUID *)CapsuleHeader, &mEfiCapsuleHeaderGuid)) {
      FoundCapsule = TRUE;
    } else if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
    	//
      // Assume the Firmware volume is a "FVMAIN" image
      //
      FoundFvMain = TRUE;
    }

    if (FoundFvMain) {
      //
      // build FV Hob if it is not built before
      //
      if (!HobUpdate) {
        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "FV Hob is not found, Build FV Hob then..\n"));

       BuildFvHob (
         (UINTN)FvHeader,
         FvHeader->FvLength
          );
      }
    }

    if (FoundCapsule) {
      //
      // Build capsule hob
      //
      Status = (*PeiServices)->CreateHob (
                                 PeiServices,
                                 EFI_HOB_TYPE_CV,
                                 sizeof (EFI_HOB_CAPSULE_VOLUME),
                                 &CapsuleHob
                                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      CapsuleHob->BaseAddress = (UINT64)((UINTN)CapsuleHeader + (UINTN)CapsuleHeader->OffsetToCapsuleBody);
      CapsuleHob->Length = (UINT64)((UINTN)CapsuleHeader->CapsuleImageSize -(UINTN)CapsuleHeader->OffsetToCapsuleBody);
      (*PeiServices)->ReportStatusCode (
                        PeiServices,
                        EFI_PROGRESS_CODE,
                        EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_CAPSULE_START,
                        0,
                        NULL,
                        NULL
                        );
    }
  }
  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Recovery Module Returning: %r\n", Status));
  return Status;
}
