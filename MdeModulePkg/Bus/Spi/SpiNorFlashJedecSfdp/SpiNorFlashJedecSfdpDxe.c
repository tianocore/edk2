/** @file
  SPI NOR Flash JEDEC Serial Flash Discoverable Parameters (SFDP)
  DXE driver.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - JEDEC Standard, JESD216F.02
      https://www.jedec.org/document_search?search_api_views_fulltext=JESD216

  @par Glossary:
    - SFDP - Serial Flash Discoverable Parameters
    - PTP  - Parameter Table Pointer
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "SpiNorFlash.h"
#include "SpiNorFlashJedecSfdpInternal.h"

/**
  Function to create SPI_NOR_FLASH_INSTANCE for this SPI part.

  @param[in] SpiIoHandle  The handle with SPI I/O protocol installed.

  @retval EFI_SUCCESS           Succeed.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource to create SPI_NOR_FLASH_INSTANCE.
  @retval otherwise             Fail to create SPI NOR Flash SFDP Instance
**/
EFI_STATUS
CreateSpiNorFlashSfdpInstance (
  IN EFI_HANDLE  SpiIoHandle
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;

  // Allocate SPI_NOR_FLASH_INSTANCE Instance.
  Instance = AllocateZeroPool (sizeof (SPI_NOR_FLASH_INSTANCE));
  ASSERT (Instance != NULL);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Locate the SPI IO Protocol
  Status = gBS->HandleProtocol (
                  SpiIoHandle,
                  &gEdk2JedecSfdpSpiDxeDriverGuid,
                  (VOID **)&Instance->SpiIo
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to locate SPI I/O protocol\n", __func__));
    FreePool (Instance);
  } else {
    Status = InitialSpiNorFlashSfdpInstance (Instance);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to initial SPI_NOR_FLASH_INSTANCE.\n", __func__));
      FreePool (Instance);
    } else {
      // Install SPI NOR Flash Protocol.
      Status = gBS->InstallProtocolInterface (
                      &Instance->Handle,
                      &gEfiSpiNorFlashProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &Instance->Protocol
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fail to Install gEfiSpiNorFlashProtocolGuid protocol.\n", __func__));
        FreePool (Instance);
      }
    }
  }

  return Status;
}

/**
  Callback function executed when the EFI_SPI_IO_PROTOCOL
  protocol interface is installed.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[out]  Context  Pointer to SPI I/O protocol GUID.

**/
VOID
EFIAPI
SpiIoProtocolInstalledCallback (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  EFI_STATUS  Status;
  UINTN       InstanceBufferSize;
  EFI_HANDLE  InstanceBuffer;

  DEBUG ((DEBUG_INFO, "%a: Entry.\n", __func__));
  InstanceBufferSize = sizeof (EFI_HANDLE);
  Status             = gBS->LocateHandle (
                              ByRegisterNotify,
                              (EFI_GUID *)Context,
                              NULL,
                              &InstanceBufferSize,
                              &InstanceBuffer
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Can't locate SPI I/O protocol.\n"));
    DEBUG ((DEBUG_INFO, "%a: Exit.\n", __func__));
    return;
  }

  CreateSpiNorFlashSfdpInstance (InstanceBuffer);
  DEBUG ((DEBUG_INFO, "%a: Exit.\n", __func__));
  return;
}

/**
  Register for the later installed SPI I/O protocol notification.

  @retval EFI_SUCCESS          Succeed.
  @retval otherwise            Fail to register SPI I/O protocol installed
                               notification.
**/
EFI_STATUS
RegisterSpioProtocolNotification (
  VOID
  )
{
  EFI_EVENT   Event;
  EFI_STATUS  Status;
  VOID        *Registration;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SpiIoProtocolInstalledCallback,
                  NULL,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to create event for the SPI I/O Protocol installation.", __func__));
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEdk2JedecSfdpSpiDxeDriverGuid,
                  Event,
                  &Registration
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to register event for the SPI I/O Protocol installation.", __func__));
  } else {
    DEBUG ((DEBUG_INFO, "%a: Notification for SPI I/O Protocol installation was registered.", __func__));
  }

  return Status;
}

/**
  Entry point of the Macronix SPI NOR Flash driver.

  @param ImageHandle  Image handle of this driver.
  @param SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS           Succeed.
  @retval EFI_NOT_FOUND         No gEdk2JedecSfdpSpiSmmDriverGuid installed on
                                system yet.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource for SPI NOR Flash JEDEC SFDP
                                initialization.
  @retval Otherwise             Other errors.
**/
EFI_STATUS
EFIAPI
SpiNorFlashJedecSfdpDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *InstanceBuffer;
  UINTN       InstanceIndex;
  UINTN       InstanceBufferSize;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  //
  // Register notification for the later SPI I/O protocol installation.
  //
  RegisterSpioProtocolNotification ();
  DEBUG ((DEBUG_INFO, "Check if there were already some gEdk2JedecSfdpSpiDxeDriverGuid handles installed.\n"));

  //
  // Check if there were already some gEdk2JedecSfdpSpiDxeDriverGuid
  // handles installed.
  //
  // Locate the SPI I/O Protocol for the SPI flash part
  // that supports JEDEC SFDP specification.
  //
  InstanceBufferSize = 0;
  InstanceBuffer     = NULL;
  Status             = gBS->LocateHandle (
                              ByProtocol,
                              &gEdk2JedecSfdpSpiDxeDriverGuid,
                              NULL,
                              &InstanceBufferSize,
                              InstanceBuffer
                              );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((
      DEBUG_INFO,
      "No gEdk2JedecSfdpSpiSmmDriverGuid handles found at the moment, wait for the notification of SPI I/O protocol installation.\n"
      ));
    DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
    return EFI_SUCCESS;
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    InstanceBuffer = (EFI_HANDLE *)AllocateZeroPool (InstanceBufferSize);
    ASSERT (InstanceBuffer != NULL);
    if (InstanceBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "Not enough resource for gEdk2JedecSfdpSpiDxeDriverGuid handles.\n"));
      DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error to locate gEdk2JedecSfdpSpiDxeDriverGuid - Status = %r.\n", Status));
    DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
    return Status;
  }

  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEdk2JedecSfdpSpiDxeDriverGuid,
                  NULL,
                  &InstanceBufferSize,
                  InstanceBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to locate all gEdk2JedecSfdpSpiDxeDriverGuid handles.\n"));
    DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%d of gEdk2JedecSfdpSpiDxeDriverGuid are found.\n", InstanceBufferSize / sizeof (EFI_HANDLE)));
  for (InstanceIndex = 0; InstanceIndex < InstanceBufferSize / sizeof (EFI_HANDLE); InstanceIndex++) {
    Status = CreateSpiNorFlashSfdpInstance (*(InstanceBuffer + InstanceIndex));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to create SPI NOR Flash SFDP instance #%d.\n", InstanceIndex));
    }
  }

  DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
  return Status;
}
