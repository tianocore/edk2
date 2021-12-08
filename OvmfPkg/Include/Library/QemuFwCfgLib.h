/** @file
  QEMU/KVM Firmware Configuration access

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FW_CFG_LIB__
#define __FW_CFG_LIB__

#include <IndustryStandard/QemuFwCfg.h>

/**
  Returns a boolean indicating if the firmware configuration interface
  is available or not.

  This function may change fw_cfg state.

  @retval    TRUE   The interface is available
  @retval    FALSE  The interface is not available

**/
BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  );

/**
  Selects a firmware configuration item for reading.

  Following this call, any data read from this item will start from
  the beginning of the configuration item's data.

  @param[in] QemuFwCfgItem - Firmware Configuration item to read

**/
VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM  QemuFwCfgItem
  );

/**
  Reads firmware configuration bytes into a buffer

  If called multiple times, then the data read will
  continue at the offset of the firmware configuration
  item where the previous read ended.

  @param[in] Size - Size in bytes to read
  @param[in] Buffer - Buffer to store data into

**/
VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer  OPTIONAL
  );

/**
  Writes firmware configuration bytes from a buffer

  If called multiple times, then the data written will
  continue at the offset of the firmware configuration
  item where the previous write ended.

  @param[in] Size - Size in bytes to write
  @param[in] Buffer - Buffer to read data from

**/
VOID
EFIAPI
QemuFwCfgWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer
  );

/**
  Skip bytes in the firmware configuration item.

  Increase the offset of the firmware configuration item without transferring
  bytes between the item and a caller-provided buffer. Subsequent read, write
  or skip operations will commence at the increased offset.

  @param[in] Size  Number of bytes to skip.
**/
VOID
EFIAPI
QemuFwCfgSkipBytes (
  IN UINTN  Size
  );

/**
  Reads a UINT8 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT8
EFIAPI
QemuFwCfgRead8 (
  VOID
  );

/**
  Reads a UINT16 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT16
EFIAPI
QemuFwCfgRead16 (
  VOID
  );

/**
  Reads a UINT32 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT32
EFIAPI
QemuFwCfgRead32 (
  VOID
  );

/**
  Reads a UINT64 firmware configuration value

  @return    Value of Firmware Configuration item read

**/
UINT64
EFIAPI
QemuFwCfgRead64 (
  VOID
  );

/**
  Find the configuration item corresponding to the firmware configuration file.

  @param[in]  Name - Name of file to look up.
  @param[out] Item - Configuration item corresponding to the file, to be passed
                     to QemuFwCfgSelectItem ().
  @param[out] Size - Number of bytes in the file.

  @return    RETURN_SUCCESS       If file is found.
             RETURN_NOT_FOUND     If file is not found.
             RETURN_UNSUPPORTED   If firmware configuration is unavailable.

**/
RETURN_STATUS
EFIAPI
QemuFwCfgFindFile (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  );

#endif
