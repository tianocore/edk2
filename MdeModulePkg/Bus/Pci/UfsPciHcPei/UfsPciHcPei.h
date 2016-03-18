/** @file
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UFS_PCI_HOST_CONTROLLER_PEI_H_
#define _UFS_PCI_HOST_CONTROLLER_PEI_H_

#include <PiPei.h>

#include <Ppi/MasterBootMode.h>
#include <Ppi/UfsHostController.h>

#include <IndustryStandard/Pci.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>

#define UFS_HC_PEI_SIGNATURE    SIGNATURE_32 ('U', 'F', 'S', 'P')
#define MAX_UFS_HCS             8

typedef struct {
  UINTN                         Signature;
  EDKII_UFS_HOST_CONTROLLER_PPI UfsHostControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR        PpiList;
  UINTN                         TotalUfsHcs;
  UINTN                         UfsHcPciAddr[MAX_UFS_HCS];
} UFS_HC_PEI_PRIVATE_DATA;

#define UFS_HC_PEI_PRIVATE_DATA_FROM_THIS(a)  CR (a, UFS_HC_PEI_PRIVATE_DATA, UfsHostControllerPpi, UFS_HC_PEI_SIGNATURE)

/**
  Get the MMIO base address of UFS host controller.

  @param[in]  This               The protocol instance pointer.
  @param[in]  ControllerId       The ID of the UFS host controller.
  @param[out] MmioBar            Pointer to the UFS host controller MMIO base address.

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
EFIAPI
GetUfsHcMmioBar (
  IN     EDKII_UFS_HOST_CONTROLLER_PPI *This,
  IN     UINT8                         ControllerId,
     OUT UINTN                         *MmioBar
  );

#endif
