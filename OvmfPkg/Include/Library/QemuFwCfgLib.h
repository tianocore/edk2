/** @file
  QEMU/KVM Firmware Configuration access

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, Red Hat, Inc.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FW_CFG_LIB__
#define __FW_CFG_LIB__

//
// The size, in bytes, of names of firmware configuration files, including at
// least one terminating NUL byte.
//
#define QEMU_FW_CFG_FNAME_SIZE 56

typedef enum {
  QemuFwCfgItemSignature            = 0x0000,
  QemuFwCfgItemInterfaceVersion     = 0x0001,
  QemuFwCfgItemSystemUuid           = 0x0002,
  QemuFwCfgItemRamSize              = 0x0003,
  QemuFwCfgItemGraphicsEnabled      = 0x0004,
  QemuFwCfgItemSmpCpuCount          = 0x0005,
  QemuFwCfgItemMachineId            = 0x0006,
  QemuFwCfgItemKernelAddress        = 0x0007,
  QemuFwCfgItemKernelSize           = 0x0008,
  QemuFwCfgItemKernelCommandLine    = 0x0009,
  QemuFwCfgItemInitrdAddress        = 0x000a,
  QemuFwCfgItemInitrdSize           = 0x000b,
  QemuFwCfgItemBootDevice           = 0x000c,
  QemuFwCfgItemNumaData             = 0x000d,
  QemuFwCfgItemBootMenu             = 0x000e,
  QemuFwCfgItemMaximumCpuCount      = 0x000f,
  QemuFwCfgItemKernelEntry          = 0x0010,
  QemuFwCfgItemKernelData           = 0x0011,
  QemuFwCfgItemInitrdData           = 0x0012,
  QemuFwCfgItemCommandLineAddress   = 0x0013,
  QemuFwCfgItemCommandLineSize      = 0x0014,
  QemuFwCfgItemCommandLineData      = 0x0015,
  QemuFwCfgItemKernelSetupAddress   = 0x0016,
  QemuFwCfgItemKernelSetupSize      = 0x0017,
  QemuFwCfgItemKernelSetupData      = 0x0018,
  QemuFwCfgItemFileDir              = 0x0019,

  QemuFwCfgItemX86AcpiTables        = 0x8000,
  QemuFwCfgItemX86SmbiosTables      = 0x8001,
  QemuFwCfgItemX86Irq0Override      = 0x8002,
  QemuFwCfgItemX86E820Table         = 0x8003,
  QemuFwCfgItemX86HpetData          = 0x8004,

} FIRMWARE_CONFIG_ITEM;


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
  IN FIRMWARE_CONFIG_ITEM   QemuFwCfgItem
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
  IN UINTN                  Size,
  IN VOID                   *Buffer  OPTIONAL
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
  IN UINTN                  Size,
  IN VOID                   *Buffer
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


/**
  Returns a boolean indicating if the firmware configuration interface is
  available for library-internal purposes.

  This function never changes fw_cfg state.

  @retval    TRUE   The interface is available internally.
  @retval    FALSE  The interface is not available internally.
**/
BOOLEAN
EFIAPI
InternalQemuFwCfgIsAvailable (
  VOID
  );


/**
  Determine if S3 support is explicitly enabled.

  @retval  TRUE   if S3 support is explicitly enabled.
           FALSE  otherwise. This includes unavailability of the firmware
                  configuration interface.
**/
BOOLEAN
EFIAPI
QemuFwCfgS3Enabled (
  VOID
  );

#endif

