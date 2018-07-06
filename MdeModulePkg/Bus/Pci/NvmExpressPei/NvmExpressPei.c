/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NvmExpressPei.h"

EFI_PEI_PPI_DESCRIPTOR  mNvmeBlkIoPpiListTemplate = {
  EFI_PEI_PPI_DESCRIPTOR_PPI,
  &gEfiPeiVirtualBlockIoPpiGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mNvmeBlkIo2PpiListTemplate = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiVirtualBlockIo2PpiGuid,
  NULL
};

EFI_PEI_NOTIFY_DESCRIPTOR  mNvmeEndOfPeiNotifyListTemplate = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NvmePeimEndOfPei
};

/**
  Check if the specified Nvm Express device namespace is active, and then get the Identify
  Namespace data.

  @param[in,out] Private        The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.
  @param[in]     NamespaceId    The specified namespace identifier.

  @retval EFI_SUCCESS    The specified namespace in the device is successfully enumerated.
  @return Others         Error occurs when enumerating the namespace.

**/
EFI_STATUS
EnumerateNvmeDevNamespace (
  IN OUT PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN UINT32                                  NamespaceId
  )
{
  EFI_STATUS                   Status;
  NVME_ADMIN_NAMESPACE_DATA    *NamespaceData;
  PEI_NVME_NAMESPACE_INFO      *NamespaceInfo;
  UINT32                       DeviceIndex;
  UINT32                       Lbads;
  UINT32                       Flbas;
  UINT32                       LbaFmtIdx;

  NamespaceData = (NVME_ADMIN_NAMESPACE_DATA *) AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));
  if (NamespaceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Identify Namespace
  //
  Status = NvmeIdentifyNamespace (
             Private,
             NamespaceId,
             NamespaceData
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: NvmeIdentifyNamespace fail, Status - %r\n", __FUNCTION__, Status));
    goto Exit;
  }

  //
  // Validate Namespace
  //
  if (NamespaceData->Ncap == 0) {
    DEBUG ((DEBUG_INFO, "%a: Namespace ID %d is an inactive one.\n", __FUNCTION__, NamespaceId));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  DeviceIndex   = Private->ActiveNamespaceNum;
  NamespaceInfo = &Private->NamespaceInfo[DeviceIndex];
  NamespaceInfo->NamespaceId   = NamespaceId;
  NamespaceInfo->NamespaceUuid = NamespaceData->Eui64;
  NamespaceInfo->Controller    = Private;
  Private->ActiveNamespaceNum++;

  //
  // Build BlockIo media structure
  //
  Flbas     = NamespaceData->Flbas;
  LbaFmtIdx = Flbas & 0xF;
  Lbads     = NamespaceData->LbaFormat[LbaFmtIdx].Lbads;

  NamespaceInfo->Media.InterfaceType  = MSG_NVME_NAMESPACE_DP;
  NamespaceInfo->Media.RemovableMedia = FALSE;
  NamespaceInfo->Media.MediaPresent   = TRUE;
  NamespaceInfo->Media.ReadOnly       = FALSE;
  NamespaceInfo->Media.BlockSize      = (UINT32) 1 << Lbads;
  NamespaceInfo->Media.LastBlock      = (EFI_PEI_LBA) NamespaceData->Nsze - 1;
  DEBUG ((
    DEBUG_INFO,
    "%a: Namespace ID %d - BlockSize = 0x%x, LastBlock = 0x%lx\n",
    __FUNCTION__,
    NamespaceId,
    NamespaceInfo->Media.BlockSize,
    NamespaceInfo->Media.LastBlock
    ));

Exit:
  if (NamespaceData != NULL) {
    FreePool (NamespaceData);
  }

  return Status;
}

/**
  Discover all Nvm Express device active namespaces.

  @param[in,out] Private    The pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS       All the namespaces in the device are successfully enumerated.
  @return EFI_NOT_FOUND     No active namespaces can be found.

**/
EFI_STATUS
NvmeDiscoverNamespaces (
  IN OUT PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private
  )
{
  UINT32    NamespaceId;

  Private->ActiveNamespaceNum = 0;
  Private->NamespaceInfo      = AllocateZeroPool (Private->ControllerData->Nn * sizeof (PEI_NVME_NAMESPACE_INFO));

  //
  // According to Nvm Express 1.1 spec Figure 82, the field 'Nn' of the identify
  // controller data defines the number of valid namespaces present for the
  // controller. Namespaces shall be allocated in order (starting with 1) and
  // packed sequentially.
  //
  for (NamespaceId = 1; NamespaceId <= Private->ControllerData->Nn; NamespaceId++) {
    //
    // For now, we do not care the return status. Since if a valid namespace is inactive,
    // error status will be returned. But we continue to enumerate other valid namespaces.
    //
    EnumerateNvmeDevNamespace (Private, NamespaceId);
  }
  if (Private->ActiveNamespaceNum == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  One notified function to cleanup the allocated resources at the end of PEI.

  @param[in] PeiServices         Pointer to PEI Services Table.
  @param[in] NotifyDescriptor    Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in] Ppi                 Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully

**/
EFI_STATUS
EFIAPI
NvmePeimEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private;

  Private = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_NOTIFY (NotifyDescriptor);
  NvmeDisableController (Private);
  NvmeFreeControllerResource (Private);

  return EFI_SUCCESS;
}

/**
  Entry point of the PEIM.

  @param[in] FileHandle     Handle of the file being invoked.
  @param[in] PeiServices    Describes the list of possible PEI Services.

  @retval EFI_SUCCESS    PPI successfully installed.

**/
EFI_STATUS
EFIAPI
NvmExpressPeimEntry (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS                               Status;
  EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI    *NvmeHcPpi;
  UINT8                                    Controller;
  UINTN                                    MmioBase;
  PEI_NVME_CONTROLLER_PRIVATE_DATA         *Private;
  EFI_PHYSICAL_ADDRESS                     DeviceAddress;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  //
  // Locate the NVME host controller PPI
  //
  Status = PeiServicesLocatePpi (
             &gEdkiiPeiNvmExpressHostControllerPpiGuid,
             0,
             NULL,
             (VOID **) &NvmeHcPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to locate NvmeHostControllerPpi.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  IoMmuInit ();

  Controller = 0;
  MmioBase   = 0;
  while (TRUE) {
    Status = NvmeHcPpi->GetNvmeHcMmioBar (
                          NvmeHcPpi,
                          Controller,
                          &MmioBase
                          );
    //
    // When status is error, meant no controller is found
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Memory allocation for controller private data
    //
    Private = AllocateZeroPool (sizeof (PEI_NVME_CONTROLLER_PRIVATE_DATA));
    if (Private == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to allocate private data for Controller %d.\n",
        __FUNCTION__,
        Controller
        ));
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Memory allocation for transfer-related data
    //
    Status = IoMmuAllocateBuffer (
               NVME_MEM_MAX_PAGES,
               &Private->Buffer,
               &DeviceAddress,
               &Private->BufferMapping
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Fail to allocate DMA buffers for Controller %d.\n",
        __FUNCTION__,
        Controller
        ));
      NvmeFreeControllerResource (Private);
      return Status;
    }
    ASSERT (DeviceAddress == ((EFI_PHYSICAL_ADDRESS) (UINTN) Private->Buffer));
    DEBUG ((DEBUG_INFO, "%a: DMA buffer base at 0x%x\n", __FUNCTION__, Private->Buffer));

    //
    // Initialize controller private data
    //
    Private->Signature = NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE;
    Private->MmioBase  = MmioBase;
    Private->BlkIoPpi.GetNumberOfBlockDevices  = NvmeBlockIoPeimGetDeviceNo;
    Private->BlkIoPpi.GetBlockDeviceMediaInfo  = NvmeBlockIoPeimGetMediaInfo;
    Private->BlkIoPpi.ReadBlocks               = NvmeBlockIoPeimReadBlocks;
    Private->BlkIo2Ppi.Revision                = EFI_PEI_RECOVERY_BLOCK_IO2_PPI_REVISION;
    Private->BlkIo2Ppi.GetNumberOfBlockDevices = NvmeBlockIoPeimGetDeviceNo2;
    Private->BlkIo2Ppi.GetBlockDeviceMediaInfo = NvmeBlockIoPeimGetMediaInfo2;
    Private->BlkIo2Ppi.ReadBlocks              = NvmeBlockIoPeimReadBlocks2;
    CopyMem (&Private->BlkIoPpiList, &mNvmeBlkIoPpiListTemplate, sizeof (EFI_PEI_PPI_DESCRIPTOR));
    CopyMem (&Private->BlkIo2PpiList, &mNvmeBlkIo2PpiListTemplate, sizeof (EFI_PEI_PPI_DESCRIPTOR));
    CopyMem (&Private->EndOfPeiNotifyList, &mNvmeEndOfPeiNotifyListTemplate, sizeof (EFI_PEI_NOTIFY_DESCRIPTOR));
    Private->BlkIoPpiList.Ppi   = &Private->BlkIoPpi;
    Private->BlkIo2PpiList.Ppi  = &Private->BlkIo2Ppi;

    //
    // Initialize the NVME controller
    //
    Status = NvmeControllerInit (Private);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Controller initialization fail for Controller %d with Status - %r.\n",
        __FUNCTION__,
        Controller,
        Status
        ));
      NvmeFreeControllerResource (Private);
      Controller++;
      continue;
    }

    //
    // Enumerate the NVME namespaces on the controller
    //
    Status = NvmeDiscoverNamespaces (Private);
    if (EFI_ERROR (Status)) {
      //
      // No active namespace was found on the controller
      //
      DEBUG ((
        DEBUG_ERROR,
        "%a: Namespaces discovery fail for Controller %d with Status - %r.\n",
        __FUNCTION__,
        Controller,
        Status
        ));
      NvmeFreeControllerResource (Private);
      Controller++;
      continue;
    }

    PeiServicesInstallPpi (&Private->BlkIoPpiList);
    PeiServicesNotifyPpi (&Private->EndOfPeiNotifyList);
    DEBUG ((
      DEBUG_INFO,
      "%a: BlockIO PPI has been installed on Controller %d.\n",
      __FUNCTION__,
      Controller
      ));
    Controller++;
  }

  return EFI_SUCCESS;
}
