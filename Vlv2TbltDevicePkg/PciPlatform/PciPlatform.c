/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


    PciPlatform.c

Abstract:
--*/


#include "PciPlatform.h"
#include "PchRegs.h"
#include "PchAccess.h"
#include "VlvCommonDefinitions.h"
#include "PlatformBootMode.h"

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/CpuIo.h>
#include <Protocol/PciIo.h>
#include <Guid/SetupVariable.h>
#include <Protocol/PciRootBridgeIo.h>
#include "SetupMode.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareVolume.h>
#include <Library/HobLib.h>
#include <IndustryStandard/Pci22.h>

extern  PCI_OPTION_ROM_TABLE  mPciOptionRomTable[];
extern  UINTN                 mSizeOptionRomTable;

EFI_PCI_PLATFORM_PROTOCOL mPciPlatform = {
  PhaseNotify,
  PlatformPrepController,
  GetPlatformPolicy,
  GetPciRom
};

EFI_HANDLE mPciPlatformHandle = NULL;


SYSTEM_CONFIGURATION          mSystemConfiguration;

EFI_STATUS
GetRawImage (
  IN EFI_GUID   *NameGuid,
  IN OUT VOID   **Buffer,
  IN OUT UINTN  *Size
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
  UINT32                        AuthenticationStatus;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolumeProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) || HandleCount == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Find desired image in all Fvs
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol(
                    HandleBuffer[Index],
                    &gEfiFirmwareVolumeProtocolGuid,
                    (VOID **) &Fv
                    );

    if ( EFI_ERROR ( Status ) ) {
      return EFI_LOAD_ERROR;
    }

    //
    // Try a raw file
    //
    *Buffer = NULL;
    *Size = 0;
    Status = Fv->ReadSection (
                   Fv,
                   NameGuid,
                   EFI_SECTION_RAW,
                   0,
                   Buffer,
                   Size,
                   &AuthenticationStatus
                   );

    if ( !EFI_ERROR ( Status )) {
        break;
    }
  }

  if ( Index >= HandleCount ) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI    
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL              *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
PlatformPrepController (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
GetPlatformPolicy (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL        *This,
  OUT EFI_PCI_PLATFORM_POLICY               *PciPolicy
  )
{
  *PciPolicy = EFI_RESERVE_VGA_IO_ALIAS;
  return EFI_SUCCESS;
}

/**
  GetPciRom from platform specific location for specific PCI device

  @param This        Protocol instance
  @param PciHandle   Identify the specific PCI devic
  @param RomImage    Returns the ROM Image memory location
  @param RomSize     Returns Rom Image size

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval  EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI    
GetPciRom (
  IN CONST EFI_PCI_PLATFORM_PROTOCOL     *This,
  IN EFI_HANDLE                           PciHandle,
  OUT  VOID                               **RomImage,
  OUT  UINTN                              *RomSize
  )
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  UINTN                         Segment;
  UINTN                         Bus;
  UINTN                         Device;
  UINTN                         Function;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT16                        DeviceClass;
  UINTN                         TableIndex;
  UINT8                         Data8;
  BOOLEAN                       MfgMode;
  EFI_PLATFORM_SETUP_ID         *BootModeBuffer;

  EFI_PEI_HOB_POINTERS        GuidHob;

  MfgMode = FALSE;

//
// Check if system is in manufacturing mode.
//
  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }

  if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformBootModeGuid, GuidHob.Raw)) != NULL) {
    BootModeBuffer = GET_GUID_HOB_DATA (GuidHob.Guid);
    if (!CompareMem (&BootModeBuffer->SetupName, MANUFACTURE_SETUP_NAME,
        StrSize (MANUFACTURE_SETUP_NAME)))
      {
      	//
        // System is in manufacturing mode.
        //
        MfgMode = TRUE;
      }
   }

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (void **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  (void **)&PciRootBridgeIo
                  );

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0x0A, 1, &DeviceClass);

  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, 1, &VendorId);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 2, 1, &DeviceId);

  //
  // WA for PCIe SATA card (SYBA SY-PEX400-40)
  //
  if ((VendorId == 0x1B21) && (DeviceId == 0x0612)) {
    Data8 = 0x07;
    PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, 4, 1, &Data8);
  }

    //
    // Do not run RAID or AHCI Option ROM if IDE
    //
    if ( (DeviceClass == ((PCI_CLASS_MASS_STORAGE << 8 ) | PCI_CLASS_MASS_STORAGE_IDE)) ) {
      return EFI_NOT_FOUND;
    }

    //
    // Run PXE ROM only if Boot network is enabled and not in MFG mode
    //
    if (DeviceClass == ((PCI_CLASS_NETWORK << 8 ) | PCI_CLASS_NETWORK_ETHERNET)) {
      if (((mSystemConfiguration.BootNetwork == 0) && (MfgMode == FALSE )) || (mSystemConfiguration.FastBoot == 1)) {
      return EFI_NOT_FOUND;
      }
    }

    //
    // Loop through table of Onboard option rom descriptions
    //
    for (TableIndex = 0; mPciOptionRomTable[TableIndex].VendorId != 0xffff; TableIndex++) {

      //
      // See if the PCI device specified by PciHandle matches at device in mPciOptionRomTable
      //
      if (VendorId != mPciOptionRomTable[TableIndex].VendorId ||
          DeviceId != mPciOptionRomTable[TableIndex].DeviceId ||
          ((DeviceClass == ((PCI_CLASS_NETWORK << 8 ) | PCI_CLASS_NETWORK_ETHERNET)) &&
           (mPciOptionRomTable[TableIndex].Flag != mSystemConfiguration.BootNetwork))  ) {
        continue;
      }

      Status = GetRawImage(
                 &mPciOptionRomTable[TableIndex].FileName,
                 RomImage,
                 RomSize
                 );

      if ((VendorId == IGD_VID) && (DeviceId == IGD_DID_VLV_A0)) {
        *(UINT16 *)(((UINTN) *RomImage) + OPROM_DID_OFFSET) = IGD_DID_VLV_A0;
      }

      if ((VendorId == IGD_VID) && (DeviceId == IGD_DID_II)) {
        *(UINT16 *)(((UINTN) *RomImage) + OPROM_DID_OFFSET) = IGD_DID_II;
      }

      if ((VendorId == IGD_VID) && (DeviceId == IGD_DID_0BE4)) {
        *(UINT16 *)(((UINTN) *RomImage) + OPROM_DID_OFFSET) = IGD_DID_0BE4;
      }

      if ((VendorId == IGD_VID) && (DeviceId == IGD_DID_QS)) {
        *(UINT16 *)(((UINTN) *RomImage) + OPROM_DID_OFFSET) = IGD_DID_QS;
      }


        if (EFI_ERROR (Status)) {
          continue;
        }
        return EFI_SUCCESS;
      }

  return EFI_NOT_FOUND;
}

/**

  @param  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

  @retval EFI_STATUS

**/
EFI_STATUS
EFIAPI
PciPlatformDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       VarSize;

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  L"Setup",
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &mSystemConfiguration
                  );
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &mSystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  

  //
  // Install on a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mPciPlatformHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatform
                  );

  return Status;
}


