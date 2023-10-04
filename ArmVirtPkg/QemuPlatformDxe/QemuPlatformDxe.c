/** @file

  The QemuPlatformDxe performs platform specific initialization.

  Copyright (c) 2018 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/VariableFormat.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FdtClient.h>

#define MAX_FLASH_BANKS  4

/** Entrypoint for QemuPlatformDxeEntryPoint

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
EFIAPI
QemuPlatformDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  INT32                Node;
  EFI_STATUS           Status;
  EFI_STATUS           FindNodeStatus;
  CONST UINT32         *Reg;
  UINT32               PropSize;
  UINT32               Num;
  UINT64               Base;
  UINT64               Size;
  BOOLEAN              Found;
  FDT_CLIENT_PROTOCOL  *FdtClient;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Num   = 0;
  Found = FALSE;
  for (FindNodeStatus = FdtClient->FindCompatibleNode (
                                     FdtClient,
                                     "cfi-flash",
                                     &Node
                                     );
       !EFI_ERROR (FindNodeStatus) && Num < MAX_FLASH_BANKS;
       FindNodeStatus = FdtClient->FindNextCompatibleNode (
                                     FdtClient,
                                     "cfi-flash",
                                     Node,
                                     &Node
                                     ))
  {
    Status = FdtClient->GetNodeProperty (
                          FdtClient,
                          Node,
                          "reg",
                          (CONST VOID **)&Reg,
                          &PropSize
                          );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: GetNodeProperty () failed (Status == %r)\n",
        __func__,
        Status
        ));
      continue;
    }

    ASSERT ((PropSize % (4 * sizeof (UINT32))) == 0);

    while (PropSize >= (4 * sizeof (UINT32)) && Num < MAX_FLASH_BANKS) {
      Base = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[0]));
      Size = SwapBytes64 (ReadUnaligned64 ((VOID *)&Reg[2]));
      Reg += 4;

      PropSize -= 4 * sizeof (UINT32);

      //
      // Disregard any flash devices that overlap with the primary FV.
      // The firmware is not updatable from inside the guest anyway.
      //
      if ((PcdGet32 (PcdOvmfFdBaseAddress) + PcdGet32 (PcdOvmfFirmwareFdSize) > Base) &&
          ((Base + Size) > PcdGet32 (PcdOvmfFdBaseAddress)))
      {
        continue;
      }

      ++Num;
      if (!Found) {
        //
        // By default, the second available flash is stored as a non-volatile variable.
        //
        Found = TRUE;
        break;
      }
    } // while
  } // for

  if (!Found) {
    DEBUG ((
      DEBUG_INFO,
      "No Flash device found, falling back to Runtime Variable Emulation\n"
      ));

    Status = PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to set PcdEmuVariableNvModeEnable, Status = %r\n",
        Status
        ));
    }

    // The driver implementing the variable service can now be dispatched.
    Status = gBS->InstallProtocolInterface (
                    &gImageHandle,
                    &gEdkiiNvVarStoreFormattedGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
