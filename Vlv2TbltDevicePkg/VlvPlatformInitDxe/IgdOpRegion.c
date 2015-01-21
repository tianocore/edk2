
/*++

Copyright (c)  2011  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  IgdOpRegion.c

Abstract:

  This is part of the implementation of an Intel Graphics drivers OpRegion /
  Software SCI interface between system BIOS, ASL code, and Graphics drivers.
  The code in this file will load the driver and initialize the interface

  Supporting Specifiction: OpRegion / Software SCI SPEC 0.70

  Acronyms:
    IGD:        Internal Graphics Device
    NVS:        ACPI Non Volatile Storage
    OpRegion:   ACPI Operational Region
    VBT:        Video BIOS Table (OEM customizable data)

--*/

//
// Include files
//


#include "IgdOpRegion.h"
#include "VlvPlatformInit.h"
#include <FrameworkDxe.h>
#include <Uefi.h>
#include <PchRegs.h>

#include <Guid/DataHubRecords.h>

#include <Protocol/IgdOpRegion.h>
#include <Protocol/FrameworkHii.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/PlatformGopPolicy.h>
#include <Protocol/PciIo.h>
#include <Protocol/CpuIo.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DriverBinding.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>



UINT8 gSVER[12] = "Intel";

extern DXE_VLV_PLATFORM_POLICY_PROTOCOL  *DxePlatformSaPolicy;

//
// Global variables
//

IGD_OPREGION_PROTOCOL mIgdOpRegion;
EFI_GUID              mMiscSubClass = EFI_MISC_SUBCLASS_GUID;
EFI_EVENT             mConOutEvent;
EFI_EVENT             mSetGOPverEvent;
VOID                  *mConOutReg;

#define DEFAULT_FORM_BUFFER_SIZE    0xFFFF
#ifndef ECP_FLAG
#if 0
/**

  Get the HII protocol interface

  @param Hii     HII protocol interface

  @retval        Status code

**/
static
EFI_STATUS
GetHiiInterface (
  OUT     EFI_HII_PROTOCOL    **Hii
  )
{
  EFI_STATUS  Status;

  //
  // There should only be one HII protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  (VOID **) Hii
                  );

  return Status;;
}
#endif
#endif

/**

  Get VBT data.

  @param[in] VbtFileBuffer    Pointer to VBT data buffer.

  @retval EFI_SUCCESS      VBT data was returned.
  @retval EFI_NOT_FOUND    VBT data not found.
  @exception EFI_UNSUPPORTED  Invalid signature in VBT data.

**/
EFI_STATUS
GetIntegratedIntelVbtPtr (
  OUT VBIOS_VBT_STRUCTURE **VbtFileBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          VbtAddress = 0;
  UINT32                        VbtSize = 0;
  UINTN                         FvProtocolCount;
  EFI_HANDLE                    *FvHandles;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
  UINTN                         Index;
  UINT32                        AuthenticationStatus;

  UINT8                         *Buffer;
  UINTN                         VbtBufferSize = 0;

  Buffer = 0;
  FvHandles      = NULL;
  *VbtFileBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeProtocolGuid,
                  NULL,
                  &FvProtocolCount,
                  &FvHandles
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < FvProtocolCount; Index++) {
      Status = gBS->HandleProtocol (
                      FvHandles[Index],
                      &gEfiFirmwareVolumeProtocolGuid,
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
        VbtAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
        VbtSize = (UINT32)VbtBufferSize;
        Status = EFI_SUCCESS;
        break;
      }
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (FvHandles != NULL) {
    FreePool(FvHandles);
    FvHandles = NULL;
  }


  //
  // Check VBT signature
  //
  *VbtFileBuffer = (VBIOS_VBT_STRUCTURE *) (UINTN) VbtAddress;
  if (*VbtFileBuffer != NULL) {
    if ((*((UINT32 *) ((*VbtFileBuffer)->HeaderSignature))) != VBT_SIGNATURE) {
      if (*VbtFileBuffer != NULL) {
        *VbtFileBuffer = NULL;
      }
      return EFI_UNSUPPORTED;
    }
    //
    // Check VBT size
    //
    if ((*VbtFileBuffer)->HeaderVbtSize > VbtBufferSize) {
      (*VbtFileBuffer)->HeaderVbtSize = (UINT16) VbtBufferSize;
    }
  }

  return EFI_SUCCESS;
}

//
// Function implementations.
//
/**

  Get a pointer to an uncompressed image of the Intel video BIOS.

  Note: This function would only be called if the video BIOS at 0xC000 is
        missing or not an Intel video BIOS.  It may not be an Intel video BIOS
        if the Intel graphic contoller is considered a secondary adapter.


  @param VBiosROMImage  Pointer to an uncompressed Intel video BIOS.  This pointer must
                        be set to NULL if an uncompressed image of the Intel Video BIOS
                        is not obtainable.


  @retval EFI_SUCCESS   VBiosPtr is updated.

**/
EFI_STATUS
GetIntegratedIntelVBiosPtr (
  INTEL_VBIOS_OPTION_ROM_HEADER **VBiosImage
  )
{
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       HandleCount;
  UINTN                       Index;
  INTEL_VBIOS_PCIR_STRUCTURE  *PcirBlockPtr;
  EFI_STATUS                  Status;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  INTEL_VBIOS_OPTION_ROM_HEADER *VBiosRomImage;

  //
  // Set as if an umcompressed Intel video BIOS image was not obtainable.
  //
  VBiosRomImage = NULL;
  *VBiosImage = NULL;

  //
  // Get all PCI IO protocols
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Find the video BIOS by checking each PCI IO handle for an Intel video
  // BIOS OPROM.
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (void **)&PciIo
                    );
    ASSERT_EFI_ERROR (Status);

    VBiosRomImage = PciIo->RomImage;

    //
    // If this PCI device doesn't have a ROM image, skip to the next device.
    //
    if (!VBiosRomImage) {
      continue;
    }

    //
    // Get pointer to PCIR structure
    //
    PcirBlockPtr = (INTEL_VBIOS_PCIR_STRUCTURE *)((UINT8 *) VBiosRomImage + VBiosRomImage->PcirOffset);

    //
    // Check if we have an Intel video BIOS OPROM.
    //
    if ((VBiosRomImage->Signature == OPTION_ROM_SIGNATURE) &&
        (PcirBlockPtr->VendorId == IGD_VID) &&
        (PcirBlockPtr->ClassCode[0] == 0x00) &&
        (PcirBlockPtr->ClassCode[1] == 0x00) &&
        (PcirBlockPtr->ClassCode[2] == 0x03)
       ) {
      //
      // Found Intel video BIOS.
      //
      *VBiosImage = VBiosRomImage;
      return EFI_SUCCESS;
    }
  }

  //
  // No Intel video BIOS found.
  //

  //
  // Free any allocated buffers
  //
  return EFI_UNSUPPORTED;
}

EFI_STATUS
SearchChildHandle(
  EFI_HANDLE Father,
  EFI_HANDLE *Child
  )
{
  EFI_STATUS            Status;
  UINTN                 HandleIndex;
  EFI_GUID              **ProtocolGuidArray = NULL;
  UINTN                 ArrayCount;
  UINTN                 ProtocolIndex;
  UINTN                 OpenInfoCount;
  UINTN                 OpenInfoIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfo = NULL;
  UINTN                 mHandleCount;
  EFI_HANDLE            *mHandleBuffer= NULL;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &mHandleCount,
                  &mHandleBuffer
                  );

  for (HandleIndex = 0; HandleIndex < mHandleCount; HandleIndex++) {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                    mHandleBuffer[HandleIndex],
                    &ProtocolGuidArray,
                    &ArrayCount
                    );
    if (!EFI_ERROR (Status)) {
      for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
        Status = gBS->OpenProtocolInformation (
                        mHandleBuffer[HandleIndex],
                        ProtocolGuidArray[ProtocolIndex],
                        &OpenInfo,
                        &OpenInfoCount
                        );
        if (!EFI_ERROR (Status)) {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if(OpenInfo[OpenInfoIndex].AgentHandle == Father) {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
                *Child = mHandleBuffer[HandleIndex];
                Status = EFI_SUCCESS;
                goto TryReturn;
              }
            }
          }
          Status = EFI_NOT_FOUND;
        }
      }
      if(OpenInfo != NULL) {
        FreePool(OpenInfo);
        OpenInfo = NULL;
      }
    }
    FreePool (ProtocolGuidArray);
    ProtocolGuidArray = NULL;
  }
TryReturn:
  if(OpenInfo != NULL) {
    FreePool (OpenInfo);
    OpenInfo = NULL;
  }
  if(ProtocolGuidArray != NULL) {
    FreePool(ProtocolGuidArray);
    ProtocolGuidArray = NULL;
  }
  if(mHandleBuffer != NULL) {
    FreePool (mHandleBuffer);
    mHandleBuffer = NULL;
  }
  return Status;
}

EFI_STATUS
JudgeHandleIsPCIDevice(
  EFI_HANDLE            Handle,
  UINT8                 Device,
  UINT8                 Funs
  )
{
  EFI_STATUS  Status;
  EFI_DEVICE_PATH   *DPath;
  EFI_DEVICE_PATH   *DevicePath;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DPath
                  );
  if(!EFI_ERROR(Status)) {
    DevicePath = DPath;
    while(!IsDevicePathEnd(DPath)) {
      if((DPath->Type == HARDWARE_DEVICE_PATH) && (DPath->SubType == HW_PCI_DP)) {
        PCI_DEVICE_PATH   *PCIPath;

        PCIPath = (PCI_DEVICE_PATH*) DPath;
        DPath = NextDevicePathNode(DPath);
        if(IsDevicePathEnd(DPath) && (PCIPath->Device == Device) && (PCIPath->Function == Funs)) {
          return EFI_SUCCESS;
        }
      } else {
        DPath = NextDevicePathNode(DPath);
      }
    }
  }
  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetDriverName(
  EFI_HANDLE   Handle,
  CHAR16         *GopVersion
  )
{
  EFI_DRIVER_BINDING_PROTOCOL           *BindHandle = NULL;
  EFI_STATUS                            Status;
  UINT32                                Version;
  UINT16                                *Ptr;

  Status = gBS->OpenProtocol(
                  Handle,
                  &gEfiDriverBindingProtocolGuid,
                  (VOID**)&BindHandle,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }

  Version = BindHandle->Version;
  Ptr = (UINT16*)&Version;
  UnicodeSPrint(GopVersion, 40, L"7.0.%04d", *(Ptr));
  return EFI_SUCCESS;
}

EFI_STATUS
GetGOPDriverVersion(
  CHAR16 *GopVersion
  )
{
  UINTN                 HandleCount;
  EFI_HANDLE            *Handles= NULL;
  UINTN                 Index;
  EFI_STATUS            Status;
  EFI_HANDLE            Child = 0;

  Status = gBS->LocateHandleBuffer(
                  ByProtocol,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  for (Index = 0; Index < HandleCount ; Index++) {
    Status = SearchChildHandle(Handles[Index], &Child);
    if(!EFI_ERROR(Status)) {
      Status = JudgeHandleIsPCIDevice(Child, 0x02, 0x00);
      if(!EFI_ERROR(Status)) {
        return GetDriverName(Handles[Index], GopVersion);
      }
    }
  }
  return EFI_UNSUPPORTED;
}


/**
  Get Intel GOP driver version and copy it into IGD OpRegion GVER. This version
  is picked up by IGD driver and displayed in CUI.

  @param  Event             A pointer to the Event that triggered the callback.
  @param  Context           A pointer to private data registered with the callback function.

  @retval EFI_SUCCESS       Video BIOS VBT information returned.
  @retval EFI_UNSUPPORTED   Could not find VBT information (*VBiosVbtPtr = NULL).

**/
EFI_STATUS
EFIAPI
SetGOPVersionCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  CHAR16                GopVersion[16] = {0};
  EFI_STATUS            Status;

  Status = GetGOPDriverVersion(GopVersion);
  if(!EFI_ERROR(Status)) {
    StrCpy((CHAR16*)&(mIgdOpRegion.OpRegion->Header.GOPV[0]), GopVersion);
    return Status;
  }
  return EFI_UNSUPPORTED;
}

/**
  Get Intel video BIOS VBT information (i.e. Pointer to VBT and VBT size).
  The VBT (Video BIOS Table) is a block of customizable data that is built
  within the video BIOS and edited by customers.

  @param  Event             A pointer to the Event that triggered the callback.
  @param  Context           A pointer to private data registered with the callback function.

  @retval EFI_SUCCESS       Video BIOS VBT information returned.
  @retval EFI_UNSUPPORTED   Could not find VBT information (*VBiosVbtPtr = NULL).

**/
EFI_STATUS
GetVBiosVbtCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  INTEL_VBIOS_PCIR_STRUCTURE    *PcirBlockPtr;
  UINT16                        PciVenderId;
  UINT16                        PciDeviceId;
  INTEL_VBIOS_OPTION_ROM_HEADER *VBiosPtr;
  VBIOS_VBT_STRUCTURE           *VBiosVbtPtr;
  VBIOS_VBT_STRUCTURE           *VbtFileBuffer = NULL;

  VBiosPtr = (INTEL_VBIOS_OPTION_ROM_HEADER *)(UINTN)(VBIOS_LOCATION_PRIMARY);
  PcirBlockPtr = (INTEL_VBIOS_PCIR_STRUCTURE *)((UINT8 *)VBiosPtr + VBiosPtr->PcirOffset);
  PciVenderId = PcirBlockPtr->VendorId;
  PciDeviceId = PcirBlockPtr->DeviceId;

  //
  // If the video BIOS is not at 0xC0000 or it is not an Intel video BIOS get
  // the integrated Intel video BIOS (must be uncompressed).
  //
  if ((VBiosPtr->Signature != OPTION_ROM_SIGNATURE) || (PciVenderId != IGD_VID) || (PciDeviceId != IGD_DID_VLV)) {
    GetIntegratedIntelVBiosPtr (&VBiosPtr);

    if(VBiosPtr) {
      //
      // Video BIOS found.
      //
      PcirBlockPtr = (INTEL_VBIOS_PCIR_STRUCTURE *)((UINT8 *)VBiosPtr + VBiosPtr->PcirOffset);
      PciVenderId = PcirBlockPtr->VendorId;
      if( (VBiosPtr->Signature != OPTION_ROM_SIGNATURE) || (PciVenderId != IGD_VID)) {
        //
        // Intel video BIOS not found.
        //
        VBiosVbtPtr = NULL;
        return EFI_UNSUPPORTED;
      }
    } else {
      //
      // No Video BIOS found, try to get VBT from FV.
      //
      GetIntegratedIntelVbtPtr (&VbtFileBuffer);
      if (VbtFileBuffer != NULL) {
        //
        // Video BIOS not found, use VBT from FV
        //
        DEBUG ((EFI_D_ERROR, "VBT data found\n"));
        (gBS->CopyMem) (
                mIgdOpRegion.OpRegion->VBT.GVD1,
                VbtFileBuffer,
                VbtFileBuffer->HeaderVbtSize
                );
        FreePool (VbtFileBuffer);
        return EFI_SUCCESS;
      }
    }
    if ((VBiosPtr == NULL) ) {
      //
      // Intel video BIOS not found.
      //
      VBiosVbtPtr = NULL;
      return EFI_UNSUPPORTED;
    }
  }

  DEBUG ((EFI_D_ERROR, "VBIOS found at 0x%X\n", VBiosPtr));
  VBiosVbtPtr = (VBIOS_VBT_STRUCTURE *) ((UINT8 *) VBiosPtr + VBiosPtr->VbtOffset);

  if ((*((UINT32 *) (VBiosVbtPtr->HeaderSignature))) != VBT_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }

  //
  // Initialize Video BIOS version with its build number.
  //
  mIgdOpRegion.OpRegion->Header.VVER[0] = VBiosVbtPtr->CoreBlockBiosBuild[0];
  mIgdOpRegion.OpRegion->Header.VVER[1] = VBiosVbtPtr->CoreBlockBiosBuild[1];
  mIgdOpRegion.OpRegion->Header.VVER[2] = VBiosVbtPtr->CoreBlockBiosBuild[2];
  mIgdOpRegion.OpRegion->Header.VVER[3] = VBiosVbtPtr->CoreBlockBiosBuild[3];
  (gBS->CopyMem) (
          mIgdOpRegion.OpRegion->VBT.GVD1,
          VBiosVbtPtr,
          VBiosVbtPtr->HeaderVbtSize
          );

  //
  // Return final status
  //
  return EFI_SUCCESS;
}

/**
  Graphics OpRegion / Software SCI driver installation function.

  @param ImageHandle     Handle for this drivers loaded image protocol.
  @param SystemTable     EFI system table.

  @retval EFI_SUCCESS    The driver installed without error.
  @retval EFI_ABORTED    The driver encountered an error and could not complete
                         installation of the ACPI tables.

**/
EFI_STATUS
IgdOpRegionInit (
  void
  )
{
  EFI_HANDLE                    Handle;
  EFI_STATUS                    Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL  *GlobalNvsArea;
  UINT32                        DwordData;
  EFI_CPU_IO_PROTOCOL           *CpuIo;
  UINT16                        Data16;
  UINT16                        AcpiBase;
  VOID                          *gConOutNotifyReg;


  //
  //  Locate the Global NVS Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiGlobalNvsAreaProtocolGuid,
                  NULL,
                  (void **)&GlobalNvsArea
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate an ACPI NVS memory buffer as the IGD OpRegion, zero initialize
  // the first 1K, and set the IGD OpRegion pointer in the Global NVS
  // area structure.
  //
  Status = (gBS->AllocatePool) (
                   EfiACPIMemoryNVS,
                   sizeof (IGD_OPREGION_STRUC),
                  (void **)&mIgdOpRegion.OpRegion
                   );
  ASSERT_EFI_ERROR (Status);
  (gBS->SetMem) (
          mIgdOpRegion.OpRegion,
          sizeof (IGD_OPREGION_STRUC),
          0
          );
  GlobalNvsArea->Area->IgdOpRegionAddress = (UINT32)(UINTN)(mIgdOpRegion.OpRegion);

  //
  // If IGD is disabled return
  //
  if (IgdMmPci32 (0) == 0xFFFFFFFF) {
    return EFI_SUCCESS;
  }

  //
  // Initialize OpRegion Header
  //

  (gBS->CopyMem) (
          mIgdOpRegion.OpRegion->Header.SIGN,
          HEADER_SIGNATURE,
          sizeof(HEADER_SIGNATURE)
          );


  //
  // Set OpRegion Size in KBs
  //
  mIgdOpRegion.OpRegion->Header.SIZE = HEADER_SIZE/1024;

  //
  // FIXME: Need to check Header OVER Field and the supported version.
  //
  mIgdOpRegion.OpRegion->Header.OVER = (UINT32) (LShiftU64 (HEADER_OPREGION_VER, 16) + LShiftU64 (HEADER_OPREGION_REV, 8));
#ifdef ECP_FLAG
  CopyMem(mIgdOpRegion.OpRegion->Header.SVER, gSVER, sizeof(gSVER));
#else
  gBS->CopyMem(
         mIgdOpRegion.OpRegion->Header.SVER,
         gSVER,
         sizeof(gSVER)
         );
#endif
  DEBUG ((EFI_D_ERROR, "System BIOS ID is %a\n", mIgdOpRegion.OpRegion->Header.SVER));


  mIgdOpRegion.OpRegion->Header.MBOX = HEADER_MBOX_SUPPORT;

  if( 1 == DxePlatformSaPolicy->IdleReserve) {
    mIgdOpRegion.OpRegion->Header.PCON = (mIgdOpRegion.OpRegion->Header.PCON & 0xFFFC) | BIT1;
  } else {
    mIgdOpRegion.OpRegion->Header.PCON = (mIgdOpRegion.OpRegion->Header.PCON & 0xFFFC) | (BIT1 | BIT0);
  }

  //
  //For graphics driver to identify if LPE Audio/HD Audio is enabled on the platform
  //
  mIgdOpRegion.OpRegion->Header.PCON &= AUDIO_TYPE_SUPPORT_MASK;
  mIgdOpRegion.OpRegion->Header.PCON &= AUDIO_TYPE_FIELD_MASK;
  if ( 1 == DxePlatformSaPolicy->AudioTypeSupport ) {
    mIgdOpRegion.OpRegion->Header.PCON = HD_AUDIO_SUPPORT;
    mIgdOpRegion.OpRegion->Header.PCON |= AUDIO_TYPE_FIELD_VALID;
  }

  //
  // Initialize OpRegion Mailbox 1 (Public ACPI Methods).
  //
  //<TODO> The initial setting of mailbox 1 fields is implementation specific.
  // Adjust them as needed many even coming from user setting in setup.
  //
  //Workaround to solve LVDS is off after entering OS in desktop platform
  //
  mIgdOpRegion.OpRegion->MBox1.CLID = DxePlatformSaPolicy->IgdPanelFeatures.LidStatus;

  //
  // Initialize OpRegion Mailbox 3 (ASLE Interrupt and Power Conservation).
  //
  //<TODO> The initial setting of mailbox 3 fields is implementation specific.
  // Adjust them as needed many even coming from user setting in setup.
  //

  //
  // Do not initialize TCHE. This field is written by the graphics driver only.
  //

  //
  // The ALSI field is generally initialized by ASL code by reading the embedded controller.
  //

  mIgdOpRegion.OpRegion->MBox3.BCLP = BACKLIGHT_BRIGHTNESS;

  mIgdOpRegion.OpRegion->MBox3.PFIT = (FIELD_VALID_BIT | PFIT_STRETCH);
  if ( DxePlatformSaPolicy->IgdPanelFeatures.PFITStatus == 2) {
  	//
    // Center
    //
    mIgdOpRegion.OpRegion->MBox3.PFIT = (FIELD_VALID_BIT | PFIT_CENTER);
  } else if (DxePlatformSaPolicy->IgdPanelFeatures.PFITStatus == 1) {
  	//
    // Stretch
    //
    mIgdOpRegion.OpRegion->MBox3.PFIT = (FIELD_VALID_BIT | PFIT_STRETCH);
  } else {
  	//
    // Auto
    //
    mIgdOpRegion.OpRegion->MBox3.PFIT = (FIELD_VALID_BIT | PFIT_SETUP_AUTO);
  }

  //
  // Set Initial current Brightness
  //
  mIgdOpRegion.OpRegion->MBox3.CBLV = (INIT_BRIGHT_LEVEL | FIELD_VALID_BIT);

  //
  // <EXAMPLE> Create a static Backlight Brightness Level Duty cycle Mapping Table
  // Possible 20 entries (example used 10), each 16 bits as follows:
  // [15] = Field Valid bit, [14:08] = Level in Percentage (0-64h), [07:00] = Desired duty cycle (0 - FFh).
  //
  //                                             %            Brightness
  mIgdOpRegion.OpRegion->MBox3.BCLM[0]   = ( (  0 << 8 ) + ( 0xFF - 0xFC ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[1]   = ( (  1 << 8 ) + ( 0xFF - 0xFC ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[2]   = ( ( 10 << 8 ) + ( 0xFF - 0xE5 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[3]   = ( ( 19 << 8 ) + ( 0xFF - 0xCE ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[4]   = ( ( 28 << 8 ) + ( 0xFF - 0xB7 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[5]   = ( ( 37 << 8 ) + ( 0xFF - 0xA0 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[6]   = ( ( 46 << 8 ) + ( 0xFF - 0x89 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[7]   = ( ( 55 << 8 ) + ( 0xFF - 0x72 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[8]   = ( ( 64 << 8 ) + ( 0xFF - 0x5B ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[9]   = ( ( 73 << 8 ) + ( 0xFF - 0x44 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[10]  = ( ( 82 << 8 ) + ( 0xFF - 0x2D ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[11]  = ( ( 91 << 8 ) + ( 0xFF - 0x16 ) + WORD_FIELD_VALID_BIT);
  mIgdOpRegion.OpRegion->MBox3.BCLM[12]  = ( (100 << 8 ) + ( 0xFF - 0x00 ) + WORD_FIELD_VALID_BIT);

  mIgdOpRegion.OpRegion->MBox3.PCFT = ((UINT32) GlobalNvsArea->Area->IgdPowerConservation) | BIT31;
  //
  // Create the notification and register callback function on the PciIo installation,
  //
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  (EFI_EVENT_NOTIFY)GetVBiosVbtCallback,
                  NULL,
                  &mConOutEvent
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;

  }

  Status = gBS->RegisterProtocolNotify (
#ifdef ECP_FLAG
                  &gExitPmAuthProtocolGuid,
#else
                  &gEfiDxeSmmReadyToLockProtocolGuid,
#endif
                  mConOutEvent,
                  &gConOutNotifyReg
                  );

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  (EFI_EVENT_NOTIFY)SetGOPVersionCallback,
                  NULL,
                  &mSetGOPverEvent
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiGraphicsOutputProtocolGuid,
                  mSetGOPverEvent,
                  &gConOutNotifyReg
                  );


  //
  // Initialize hardware state:
  //   Set ASLS Register to the OpRegion physical memory address.
  //   Set SWSCI register bit 15 to a "1" to activate SCI interrupts.
  //

  IgdMmPci32 (IGD_ASLS_OFFSET) = (UINT32)(UINTN)(mIgdOpRegion.OpRegion);
  IgdMmPci16AndThenOr (IGD_SWSCI_OFFSET, ~(BIT0), BIT15);

  DwordData = IgdMmPci32 (IGD_ASLS_OFFSET);
  S3BootScriptSavePciCfgWrite (
    S3BootScriptWidthUint32,
    (UINTN) (EFI_PCI_ADDRESS  (IGD_BUS, IGD_DEV, IGD_FUN_0, IGD_ASLS_OFFSET)),
    1,
    &DwordData
    );


  DwordData = IgdMmPci32 (IGD_SWSCI_OFFSET);
  S3BootScriptSavePciCfgWrite (
    S3BootScriptWidthUint32,
    (UINTN) (EFI_PCI_ADDRESS  (IGD_BUS, IGD_DEV, IGD_FUN_0, IGD_SWSCI_OFFSET)),
    1,
    &DwordData
    );

  AcpiBase =  MmPci16 (
                0,
                DEFAULT_PCI_BUS_NUMBER_PCH,
                PCI_DEVICE_NUMBER_PCH_LPC,
                PCI_FUNCTION_NUMBER_PCH_LPC,
                R_PCH_LPC_ACPI_BASE
                ) & B_PCH_LPC_ACPI_BASE_BAR;

  //
  // Find the CPU I/O Protocol.  ASSERT if not found.
  //
  Status = gBS->LocateProtocol (
                  &gEfiCpuIoProtocolGuid,
                  NULL,
                  (void **)&CpuIo
                  );
  ASSERT_EFI_ERROR (Status);

  CpuIo->Io.Read (
              CpuIo,
              EfiCpuIoWidthUint16,
              AcpiBase + R_PCH_ACPI_GPE0a_STS,
              1,
              &Data16
              );
  //
  // Clear the B_PCH_ACPI_GPE0a_STS_GUNIT_SCI bit in R_PCH_ACPI_GPE0a_STS by writing a '1'.
  //
  Data16 |= B_PCH_ACPI_GPE0a_STS_GUNIT_SCI;

  CpuIo->Io.Write (
              CpuIo,
              EfiCpuIoWidthUint16,
              AcpiBase + R_PCH_ACPI_GPE0a_STS,
              1,
              &Data16
              );

  //
  // Install OpRegion / Software SCI protocol
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gIgdOpRegionProtocolGuid,
                  &mIgdOpRegion,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Return final status
  //
  return EFI_SUCCESS;
}
