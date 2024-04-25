/** @file
  SPI NOR Flash JEDEC Serial Flash Discoverable Parameters (SFDP)
  SMM driver.

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
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SpiSmmConfiguration.h>
#include <Protocol/SpiSmmNorFlash.h>
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

  // Locate the SPI IO Protocol.
  Status = gSmst->SmmHandleProtocol (
                    SpiIoHandle,
                    &gEdk2JedecSfdpSpiSmmDriverGuid,
                    (VOID **)&Instance->SpiIo
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to locate SPI I/O protocol.\n", __func__));
    FreePool (Instance);
  } else {
    Status = InitialSpiNorFlashSfdpInstance (Instance);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to initial SPI_NOR_FLASH_INSTANCE.\n", __func__));
      FreePool (Instance);
    } else {
      // Install SPI NOR Flash Protocol.
      Status = gSmst->SmmInstallProtocolInterface (
                        &Instance->Handle,
                        &gEfiSpiSmmNorFlashProtocolGuid,
                        EFI_NATIVE_INTERFACE,
                        &Instance->Protocol
                        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fail to Install gEfiSpiSmmNorFlashProtocolGuid protocol.\n", __func__));
        FreePool (Instance);
      }
    }
  }

  return Status;
}

/**
  Callback function executed when the EFI_SPI_IO_PROTOCOL
  protocol interface is installed.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SpiIoProtocolInstalledCallback (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a: Entry.\n", __func__));
  Status = CreateSpiNorFlashSfdpInstance (Handle);
  return Status;
}

/**
  Register notification for the later installed SPI I/O protocol.

  @retval EFI_SUCCESS          Succeed.
  @retval otherwise            Fail to register the notification of
                               SPI I/O protocol installation.

**/
EFI_STATUS
RegisterSpioProtocolNotification (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEdk2JedecSfdpSpiSmmDriverGuid,
                    SpiIoProtocolInstalledCallback,
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
  Entry point of the SPI NOR Flash SFDP SMM driver.

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
SpiNorFlashJedecSfdpSmmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *InstanceBuffer;
  UINTN       InstanceIndex;
  UINTN       InstanceBufferSize;

  DEBUG ((DEBUG_INFO, "%a - ENTRY.\n", __func__));

  //
  // Register notification for the later SPI I/O protocol installation.
  //
  RegisterSpioProtocolNotification ();
  DEBUG ((DEBUG_INFO, "Check if there were already some gEdk2JedecSfdpSpiSmmDriverGuid handles installed.\n"));
  //
  // Check if there were already some gEdk2JedecSfdpSpiSmmDriverGuid
  // handles installed.
  //
  // Locate the SPI I/O Protocol for the SPI flash part
  // that supports JEDEC SFDP specification.
  //
  InstanceBufferSize = 0;
  InstanceBuffer     = NULL;
  Status             = gSmst->SmmLocateHandle (
                                ByProtocol,
                                &gEdk2JedecSfdpSpiSmmDriverGuid,
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
      DEBUG ((DEBUG_ERROR, "Not enough resource for gEdk2JedecSfdpSpiSmmDriverGuid handles.\n"));
      DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error to locate gEdk2JedecSfdpSpiSmmDriverGuid - Status = %r.\n", Status));
    DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
    return Status;
  }

  Status = gSmst->SmmLocateHandle (
                    ByProtocol,
                    &gEdk2JedecSfdpSpiSmmDriverGuid,
                    NULL,
                    &InstanceBufferSize,
                    InstanceBuffer
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fail to locate all gEdk2JedecSfdpSpiSmmDriverGuid handles.\n"));
    DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%d of gEdk2JedecSfdpSpiSmmDriverGuid handles are found.\n", InstanceBufferSize / sizeof (EFI_HANDLE)));
  for (InstanceIndex = 0; InstanceIndex < InstanceBufferSize / sizeof (EFI_HANDLE); InstanceIndex++) {
    Status = CreateSpiNorFlashSfdpInstance (*(InstanceBuffer + InstanceIndex));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to create SPI NOR Flash SFDP instance #%d.\n", InstanceIndex));
    }
  }

  DEBUG ((DEBUG_INFO, "%a: EXIT - Status=%r\n", __func__, Status));
  return Status;
}
