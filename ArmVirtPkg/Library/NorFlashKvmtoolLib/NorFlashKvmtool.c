/** @file
   An instance of the NorFlashPlatformLib for Kvmtool platform.

 Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

/** Macro defining the NOR block size configured in Kvmtool.
*/
#define KVMTOOL_NOR_BLOCK_SIZE  SIZE_64KB

/** Macro defining the maximum number of Flash devices.
*/
#define MAX_FLASH_DEVICES       4

/** Macro defining the cfi-flash label describing the UEFI variable store.
*/
#define LABEL_UEFI_VAR_STORE    "System-firmware"

STATIC NOR_FLASH_DESCRIPTION  mNorFlashDevices[MAX_FLASH_DEVICES];
STATIC UINTN                  mNorFlashDeviceCount = 0;
STATIC INT32                  mUefiVarStoreNode = MAX_INT32;
STATIC FDT_CLIENT_PROTOCOL    *mFdtClient;

/** This function performs platform specific actions to initialise
    the NOR flash, if required.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  EFI_STATUS                  Status;

  DEBUG ((DEBUG_INFO, "NorFlashPlatformInitialization\n"));

  if ((mNorFlashDeviceCount > 0) && (mUefiVarStoreNode != MAX_INT32)) {
    //
    // UEFI takes ownership of the cfi-flash hardware, and exposes its
    // functionality through the UEFI Runtime Variable Service. This means we
    // need to disable it in the device tree to prevent the OS from attaching
    // its device driver as well.
    // Note: This library is loaded twice. First by FaultTolerantWriteDxe to
    // setup the PcdFlashNvStorageFtw* and later by NorFlashDxe to provide the
    // NorFlashPlatformLib interfaces. If the node is disabled when the library
    // is first loaded, then during the subsequent loading of the library the
    // call to FindNextCompatibleNode() from the library constructor skips the
    // FDT node used for UEFI storage variable. Due to this we cannot setup the
    // NOR flash device description i.e. mNorFlashDevices[].
    // Since NorFlashPlatformInitialization() is called only by NorFlashDxe,
    // we know it is safe to disable the node here.
    //
    Status = mFdtClient->SetNodeProperty (
                           mFdtClient,
                           mUefiVarStoreNode,
                           "status",
                           "disabled",
                           sizeof ("disabled")
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to set cfi-flash status to 'disabled'\n"));
    }
  } else {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "Flash device for UEFI variable storage not found\n"));
  }

  return Status;
}

/** Initialise Non volatile Flash storage variables.

  @param [in]  FlashDevice Pointer to the NOR Flash device.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Insufficient flash storage space.
**/
STATIC
EFI_STATUS
SetupVariableStore (
  IN NOR_FLASH_DESCRIPTION * FlashDevice
  )
{
  UINTN   FlashRegion;
  UINTN   FlashNvStorageVariableBase;
  UINTN   FlashNvStorageFtwWorkingBase;
  UINTN   FlashNvStorageFtwSpareBase;
  UINTN   FlashNvStorageVariableSize;
  UINTN   FlashNvStorageFtwWorkingSize;
  UINTN   FlashNvStorageFtwSpareSize;

  FlashNvStorageVariableSize = PcdGet32 (PcdFlashNvStorageVariableSize);
  FlashNvStorageFtwWorkingSize = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  FlashNvStorageFtwSpareSize =  PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  if ((FlashNvStorageVariableSize == 0)   ||
      (FlashNvStorageFtwWorkingSize == 0) ||
      (FlashNvStorageFtwSpareSize == 0)) {
    DEBUG ((DEBUG_ERROR, "FlashNvStorage size not defined\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Setup the variable store
  FlashRegion = FlashDevice->DeviceBaseAddress;

  FlashNvStorageVariableBase = FlashRegion;
  FlashRegion += PcdGet32 (PcdFlashNvStorageVariableSize);

  FlashNvStorageFtwWorkingBase = FlashRegion;
  FlashRegion += PcdGet32 (PcdFlashNvStorageFtwWorkingSize);

  FlashNvStorageFtwSpareBase = FlashRegion;
  FlashRegion += PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  if (FlashRegion > (FlashDevice->DeviceBaseAddress + FlashDevice->Size)) {
    DEBUG ((DEBUG_ERROR, "Insufficient flash storage size\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  PcdSet32S (
    PcdFlashNvStorageVariableBase,
    FlashNvStorageVariableBase
    );

  PcdSet32S (
    PcdFlashNvStorageFtwWorkingBase,
    FlashNvStorageFtwWorkingBase
    );

  PcdSet32S (
    PcdFlashNvStorageFtwSpareBase,
    FlashNvStorageFtwSpareBase
    );

  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageVariableBase = 0x%x\n",
    FlashNvStorageVariableBase
    ));
  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageVariableSize = 0x%x\n",
    FlashNvStorageVariableSize
    ));
  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageFtwWorkingBase = 0x%x\n",
    FlashNvStorageFtwWorkingBase
    ));
  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageFtwWorkingSize = 0x%x\n",
    FlashNvStorageFtwWorkingSize
    ));
  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageFtwSpareBase = 0x%x\n",
    FlashNvStorageFtwSpareBase
    ));
  DEBUG ((
    DEBUG_INFO,
    "PcdFlashNvStorageFtwSpareSize = 0x%x\n",
    FlashNvStorageFtwSpareSize
    ));

  return EFI_SUCCESS;
}

/** Return the Flash devices on the platform.

  @param [out]  NorFlashDescriptions    Pointer to the Flash device description.
  @param [out]  Count                   Number of Flash devices.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         Flash device not found.
**/
EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDescriptions,
  OUT UINT32                  *Count
  )
{
  if (mNorFlashDeviceCount > 0) {
    *NorFlashDescriptions = mNorFlashDevices;
    *Count = mNorFlashDeviceCount;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

/** Entrypoint for NorFlashPlatformLib.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Flash device not found.
**/
EFI_STATUS
EFIAPI
NorFlashPlatformLibConstructor (
  IN  EFI_HANDLE          ImageHandle,
  IN  EFI_SYSTEM_TABLE  * SystemTable
  )
{
  INT32                       Node;
  EFI_STATUS                  Status;
  EFI_STATUS                  FindNodeStatus;
  CONST UINT32                *Reg;
  UINT32                      PropSize;
  UINT64                      Base;
  UINT64                      Size;
  UINTN                       UefiVarStoreIndex;
  CONST CHAR8                 *Label;
  UINT32                      LabelLen;

  if (mNorFlashDeviceCount != 0) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&mFdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  UefiVarStoreIndex = MAX_UINTN;
  for (FindNodeStatus = mFdtClient->FindCompatibleNode (
                                      mFdtClient,
                                      "cfi-flash",
                                      &Node
                                      );
       !EFI_ERROR (FindNodeStatus) &&
         (mNorFlashDeviceCount < MAX_FLASH_DEVICES);
       FindNodeStatus = mFdtClient->FindNextCompatibleNode (
                                      mFdtClient,
                                      "cfi-flash",
                                      Node,
                                      &Node
    )) {
    Status = mFdtClient->GetNodeProperty (
                           mFdtClient,
                           Node,
                           "label",
                           (CONST VOID **)&Label,
                           &LabelLen
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: GetNodeProperty ('label') failed (Status == %r)\n",
        __FUNCTION__,
        Status
        ));
    } else if (AsciiStrCmp (Label, LABEL_UEFI_VAR_STORE) == 0) {
      UefiVarStoreIndex = mNorFlashDeviceCount;
      mUefiVarStoreNode = Node;
    }

    Status = mFdtClient->GetNodeProperty (
                           mFdtClient,
                           Node,
                           "reg",
                           (CONST VOID **)&Reg,
                           &PropSize
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: GetNodeProperty () failed (Status == %r)\n",
        __FUNCTION__, Status));
      continue;
    }

    ASSERT ((PropSize % (4 * sizeof (UINT32))) == 0);

    while ((PropSize >= (4 * sizeof (UINT32))) &&
           (mNorFlashDeviceCount < MAX_FLASH_DEVICES)) {
      Base = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
      Size = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[2]));
      Reg += 4;

      PropSize -= 4 * sizeof (UINT32);

      //
      // Disregard any flash devices that overlap with the primary FV.
      // The firmware is not updatable from inside the guest anyway.
      //
      if ((PcdGet64 (PcdFvBaseAddress) + PcdGet32 (PcdFvSize) > Base) &&
          (Base + Size) > PcdGet64 (PcdFvBaseAddress)) {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "NOR%d : Base = 0x%lx, Size = 0x%lx\n",
        mNorFlashDeviceCount,
        Base,
        Size
        ));

      mNorFlashDevices[mNorFlashDeviceCount].DeviceBaseAddress = (UINTN)Base;
      mNorFlashDevices[mNorFlashDeviceCount].RegionBaseAddress = (UINTN)Base;
      mNorFlashDevices[mNorFlashDeviceCount].Size = (UINTN)Size;
      mNorFlashDevices[mNorFlashDeviceCount].BlockSize = KVMTOOL_NOR_BLOCK_SIZE;
      mNorFlashDeviceCount++;
    }
  } // for

  // Setup the variable store in the last device
  if (mNorFlashDeviceCount > 0) {
    if (UefiVarStoreIndex == MAX_UINTN) {
      // We did not find a label matching the UEFI Variable store. Default to
      // using the last cfi-flash device as the variable store.
      UefiVarStoreIndex = mNorFlashDeviceCount - 1;
      mUefiVarStoreNode = Node;
    }
    if (mNorFlashDevices[UefiVarStoreIndex].DeviceBaseAddress != 0) {
      return SetupVariableStore (&mNorFlashDevices[UefiVarStoreIndex]);
    }
  }

  return EFI_NOT_FOUND;
}
