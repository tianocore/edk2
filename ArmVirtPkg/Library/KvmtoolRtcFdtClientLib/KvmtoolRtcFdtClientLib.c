/** @file
  FDT client library for motorola,mc146818 RTC driver

  Copyright (c) 2020, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

/** RTC Index register is at offset 0x0
*/
#define RTC_INDEX_REG_OFFSET  0x0ULL

/** RTC Target register is at offset 0x1
*/
#define RTC_TARGET_REG_OFFSET  0x1ULL

/** Add the RTC controller address range to the memory map.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  RtcPageBase  Base address of the RTC controller.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Flash device not found.
**/
STATIC
EFI_STATUS
KvmtoolRtcMapMemory (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_PHYSICAL_ADDRESS  RtcPageBase
  )
{
  EFI_STATUS  Status;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  RtcPageBase,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to add memory space. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  0,
                  EFI_PAGE_SIZE,
                  &RtcPageBase,
                  ImageHandle,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to allocate memory space. Status = %r\n",
      Status
      ));
    gDS->RemoveMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  RtcPageBase,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME | EFI_MEMORY_XP
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to set memory attributes. Status = %r\n",
      Status
      ));
    gDS->FreeMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );
    gDS->RemoveMemorySpace (
           RtcPageBase,
           EFI_PAGE_SIZE
           );
  }

  return Status;
}

/** Entrypoint for KvmtoolRtcFdtClientLib.

  Locate the RTC node in the DT and update the Index and
  Target register base addresses in the respective PCDs.
  Add the RTC memory region to the memory map.
  Disable the RTC node as the RTC is owned by UEFI.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Flash device not found.
**/
EFI_STATUS
EFIAPI
KvmtoolRtcFdtClientLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST UINT32         *Reg;
  UINT32               RegSize;
  UINT64               RegBase;
  UINT64               Range;
  RETURN_STATUS        PcdStatus;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNode (
                        FdtClient,
                        "motorola,mc146818",
                        &Node
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No 'motorola,mc146818' compatible DT node found\n",
      __func__
      ));
    return Status;
  }

  Status = FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "reg",
                        (CONST VOID **)&Reg,
                        &RegSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No 'reg' property found in 'motorola,mc146818' compatible DT node\n",
      __func__
      ));
    return Status;
  }

  ASSERT (RegSize == 16);

  RegBase = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
  Range   = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[2]));
  DEBUG ((
    DEBUG_INFO,
    "Found motorola,mc146818 RTC @ 0x%Lx Range = 0x%x\n",
    RegBase,
    Range
    ));

  // The address range must cover the RTC Index and the Target registers.
  ASSERT (Range >= 0x2);

  // RTC Index register is at offset 0x0
  PcdStatus = PcdSet64S (
                PcdRtcIndexRegister64,
                (RegBase + RTC_INDEX_REG_OFFSET)
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  // RTC Target register is at offset 0x1
  PcdStatus = PcdSet64S (
                PcdRtcTargetRegister64,
                (RegBase + RTC_TARGET_REG_OFFSET)
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  Status = KvmtoolRtcMapMemory (ImageHandle, (RegBase & ~EFI_PAGE_MASK));
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to map memory for motorola,mc146818. Status = %r\n",
      Status
      ));
    return Status;
  }

  //
  // UEFI takes ownership of the RTC hardware, and exposes its functionality
  // through the UEFI Runtime Services GetTime, SetTime, etc. This means we
  // need to disable it in the device tree to prevent the OS from attaching
  // its device driver as well.
  //
  Status = FdtClient->SetNodeProperty (
                        FdtClient,
                        Node,
                        "status",
                        "disabled",
                        sizeof ("disabled")
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "Failed to set motorola,mc146818 status to 'disabled', Status = %r\n",
      Status
      ));
  }

  return EFI_SUCCESS;
}
