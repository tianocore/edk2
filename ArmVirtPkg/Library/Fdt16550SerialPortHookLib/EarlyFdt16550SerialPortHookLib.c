/** @file
  Early Platform Hook Library instance for 16550 Uart.

  Copyright (c) 2020, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformHookLib.h>
#include <libfdt.h>

/** Get the UART base address of the console serial-port from the DT.

  This function fetches the node referenced in the "stdout-path"
  property of the "chosen" node and returns the base address of
  the console UART.

  @param [in]   Fdt                   Pointer to a Flattened Device Tree (Fdt).
  @param [out]  SerialConsoleAddress  If success, contains the base address
                                      of the console serial-port.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           Console serial-port info not found in DT.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GetSerialConsolePortAddress (
  IN  CONST VOID    *Fdt,
  OUT       UINT64  *SerialConsoleAddress
  )
{
  CONST CHAR8   *Prop;
  INT32         PropSize;
  CONST CHAR8   *Path;
  INT32         PathLen;
  INT32         ChosenNode;
  INT32         SerialConsoleNode;
  INT32         Len;
  CONST CHAR8   *NodeStatus;
  CONST UINT64  *RegProperty;

  if ((Fdt == NULL) || (fdt_check_header (Fdt) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // The "chosen" node resides at the root of the DT. Fetch it.
  ChosenNode = fdt_path_offset (Fdt, "/chosen");
  if (ChosenNode < 0) {
    return EFI_NOT_FOUND;
  }

  Prop = fdt_getprop (Fdt, ChosenNode, "stdout-path", &PropSize);
  if (PropSize < 0) {
    return EFI_NOT_FOUND;
  }

  // Determine the actual path length, as a colon terminates the path.
  Path = ScanMem8 (Prop, ':', PropSize);
  if (Path == NULL) {
    PathLen = AsciiStrLen (Prop);
  } else {
    PathLen = Path - Prop;
  }

  // Aliases cannot start with a '/', so it must be the actual path.
  if (Prop[0] == '/') {
    SerialConsoleNode = fdt_path_offset_namelen (Fdt, Prop, PathLen);
  } else {
    // Lookup the alias, as this contains the actual path.
    Path = fdt_get_alias_namelen (Fdt, Prop, PathLen);
    if (Path == NULL) {
      return EFI_NOT_FOUND;
    }

    SerialConsoleNode = fdt_path_offset (Fdt, Path);
  }

  NodeStatus = fdt_getprop (Fdt, SerialConsoleNode, "status", &Len);
  if ((NodeStatus != NULL) && (AsciiStrCmp (NodeStatus, "okay") != 0)) {
    return EFI_NOT_FOUND;
  }

  RegProperty = fdt_getprop (Fdt, SerialConsoleNode, "reg", &Len);
  if (Len != 16) {
    return EFI_INVALID_PARAMETER;
  }

  *SerialConsoleAddress = fdt64_to_cpu (ReadUnaligned64 (RegProperty));

  return EFI_SUCCESS;
}

/** Platform hook to retrieve the 16550 UART base address from the platform
    Device tree and store it in PcdSerialRegisterBase.

  @retval RETURN_SUCCESS            Success.
  @retval RETURN_INVALID_PARAMETER  A parameter was invalid.
  @retval RETURN_NOT_FOUND          Serial port information not found.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS  Status;
  VOID           *DeviceTreeBase;
  UINT64         SerialConsoleAddress;

  if (PcdGet64 (PcdSerialRegisterBase) != 0) {
    return RETURN_SUCCESS;
  }

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  if (DeviceTreeBase == NULL) {
    return RETURN_NOT_FOUND;
  }

  Status = GetSerialConsolePortAddress (DeviceTreeBase, &SerialConsoleAddress);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  return (EFI_STATUS)PcdSet64S (PcdSerialRegisterBase, SerialConsoleAddress);
}
