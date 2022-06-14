/** @file
  Implementation for a generic i801 SMBus driver.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "SMBusConfigLoader.h"
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/SmbusLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>

/**
  GetPciConfigSpaceAddress

  Return the PCI Config Space Address if found, zero otherwise.

  @retval 0                 Can not find any SMBusController
  @retval UINT32            PCI Config Space Address
**/
STATIC UINT32
EFIAPI
GetPciConfigSpaceAddress(
  )
{
  UINT8                     Device;
  UINT8                     Function;
  UINT8                     BaseClass;
  UINT8                     SubClass;

  //
  // Search for SMBus Controller within PCI Devices on root bus
  //
  for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
    for (Function = 0; Function <= PCI_MAX_FUNC; Function++) {
      if (PciRead16(PCI_LIB_ADDRESS(0, Device, Function, 0x00)) != 0x8086)
        continue;

      BaseClass = PciRead8(PCI_LIB_ADDRESS(0, Device, Function, 0x0B));

      if (BaseClass == PCI_CLASS_SERIAL) {
        SubClass = PciRead8(PCI_LIB_ADDRESS(0, Device, Function, 0xA));

        if (SubClass == PCI_CLASS_SERIAL_SMB) {
          return PCI_LIB_ADDRESS(0, Device, Function, 0x00);
        }
      }
    }
  }

  return 0;
}

/**
  ReadBoardOptionFromEEPROM

  Reads the Board options like Primary Video from the EEPROM

  @param Buffer         Pointer to the Buffer Array

**/
STATIC
EFI_STATUS
EFIAPI
ReadBoardOptionFromEEPROM (
  IN OUT UINT8          *Buffer,
  IN     UINT32          Size
  )
{
  EFI_STATUS                Status;
  UINT16                    Index;
  UINT16                    Value;

  for (Index = BOARD_SETTINGS_OFFSET; Index < BOARD_SETTINGS_OFFSET + Size; Index += 2) {
    Value = SmBusProcessCall(SMBUS_LIB_ADDRESS(0x57, 0, 0, 0), ((Index & 0xff) << 8) | ((Index & 0xff00) >> 8), &Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to read SMBUS byte at offset 0x%x\n", Index));
      return Status;
    }
    CopyMem(&Buffer[Index-BOARD_SETTINGS_OFFSET], &Value, sizeof(Value));
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WriteToEEPROM(const IN UINT8 Data, const IN UINT16 Offset) {
  EFI_STATUS Status;

  UINT8 buffer[2] = {
    Offset & 0xff,
    Data,
  };

  for (INTN retry = 3; retry > 0; retry--) {
    SmBusWriteBlock(SMBUS_LIB_ADDRESS(0x57, Offset >> 8, sizeof(buffer), 0),
                    buffer, &Status);
    if (!EFI_ERROR(Status))
      break;
    /* Maximum of 5 milliseconds write duration */
    MicroSecondDelay(5000);
  }

  if (EFI_ERROR(Status)) {
    DEBUG(
          (DEBUG_ERROR, "Failed to write SMBUS byte at offset 0x%x\n", Offset));
    return Status;
  }
  return EFI_SUCCESS;
  }

/**
  ReadBootOverrideFromEEPROM

  @param Buffer         Pointer to the Buffer Array

**/
STATIC
EFI_STATUS
EFIAPI
ReadBootOverrideFromEEPROM(IN OUT UINT8 *Buffer, IN UINT32 Size) {
  EFI_STATUS Status;
  UINT16 Index;
  UINT16 Value;

  for (Index = BOARD_BOOT_OVERRIDE_OFFSET;
       Index < BOARD_BOOT_OVERRIDE_OFFSET + Size; Index += 2) {
    Value = SmBusProcessCall(SMBUS_LIB_ADDRESS(0x57, 0, 0, 0),
                             ((Index & 0xff) << 8) | ((Index & 0xff00) >> 8),
                             &Status);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to read SMBUS byte at offset 0x%x\n", Index));
      return Status;
    }
    CopyMem(&Buffer[Index - BOARD_BOOT_OVERRIDE_OFFSET], &Value, sizeof(Value));
  }
  return EFI_SUCCESS;
}

STATIC
UINT32
EFIAPI
crc32_byte(UINT32 prev_crc, UINT8 data)
{
  prev_crc ^= (UINT32)data << 24;

  for (int i = 0; i < 8; i++) {
    if ((prev_crc & 0x80000000UL) != 0)
      prev_crc = ((prev_crc << 1) ^ 0x04C11DB7UL);
    else
      prev_crc <<= 1;
  }

  return prev_crc;
}


/**
  Computes and returns a 32-bit CRC for a data buffer.
  CRC32 value bases on ITU-T V.42.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param[in]  Buffer       A pointer to the buffer on which the 32-bit CRC is to be computed.
  @param[in]  Length       The number of bytes in the buffer Data.

  @retval Crc32            The 32-bit CRC was computed for the data buffer.

**/
STATIC
UINT32
EFIAPI
CalculateCrc32(
  IN  VOID                         *Buffer,
  IN  UINTN                        Length
  )
{
  int i;
  UINT32 crc = 0x0;

  for ( i = 0; i < Length; i++ )
  {
    crc = crc32_byte(crc, ((UINT8 *)Buffer)[i]);
  }

  return crc;
}

/**
  The Entry Point for SMBUS driver.

  It installs DriverBinding.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InstallSMBusConfigLoader (
  IN EFI_HANDLE                        ImageHandle,
  IN EFI_SYSTEM_TABLE                  *SystemTable
  )
{
  EFI_STATUS                Status;
  BOARD_SETTINGS            BoardSettings;
  UINT32                    BaseAddress;
  UINT32                    CRC32Array;
  UINT32                    HostCBackup;
  UINT8                     Array[sizeof(BOARD_SETTINGS)];
  BOARD_BOOT_OVERRIDE       BoardBootOverride;
  BOOT_OVERRIDE             BootOverride;
  RETURN_STATUS             PcdStatus;

  DEBUG ((DEBUG_INFO, "SMBusConfigLoader: InstallSMBusConfigLoader\n"));

  BaseAddress = GetPciConfigSpaceAddress();
  if (BaseAddress == 0) {
    return EFI_NOT_FOUND;
  }

  // Always clear the Boot Override.
  Status =
      gRT->SetVariable(BOARD_BOOT_OVERRIDE_NAME,           // Variable Name
                       &gEfiBoardBootOverrideVariableGuid, // Variable Guid
                       (EFI_VARIABLE_BOOTSERVICE_ACCESS), // Variable Attributes
                       sizeof(BootOverride), &BootOverride);

  ZeroMem(&BoardSettings, sizeof(BOARD_SETTINGS));

  // Set I2C_EN Bit
  HostCBackup = PciRead32(BaseAddress + HOSTC);
  PciWrite32(BaseAddress + HOSTC, HostCBackup | I2C_EN_HOSTC);

  Status = ReadBoardOptionFromEEPROM(Array, sizeof(Array));

  CRC32Array = 0;
  if (!EFI_ERROR(Status)) {
    CopyMem(&BoardSettings, Array, sizeof(BOARD_SETTINGS));

    DEBUG ((DEBUG_INFO, "SMBusConfigLoader: Board Settings:\n"));
    DEBUG ((DEBUG_INFO, "SMBusConfigLoader: CRC: %08x - SecureBoot: %02x - PrimaryVideo: %02x - BIOS menu disabled: %02x\n",
      BoardSettings.Signature, BoardSettings.SecureBoot, BoardSettings.PrimaryVideo, BoardSettings.MenuDisabled));

    CRC32Array = CalculateCrc32(&Array[sizeof(UINT32)], sizeof(BOARD_SETTINGS) - sizeof(UINT32));
    if (CRC32Array != BoardSettings.Signature)
    {
      DEBUG ((DEBUG_ERROR, "SMBusConfigLoader: Checksum invalid. Should be %04X - is: %04x.\nReseting to defaults.\n", CRC32Array, BoardSettings.Signature));
    }
  }
  if (EFI_ERROR(Status) || (CRC32Array != BoardSettings.Signature)) {
    BoardSettings.PrimaryVideo = 0;
    BoardSettings.SecureBoot = 1;
    BoardSettings.MenuDisabled = 0;
  }

  /* Maximum of 5 milliseconds */
  MicroSecondDelay(5000);

  Status = ReadBootOverrideFromEEPROM((UINT8 *)&BoardBootOverride, sizeof(BoardBootOverride));
  if (!EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "SMBusConfigLoader: Boot Override:\n"));
    DEBUG((DEBUG_INFO,
           "SMBusConfigLoader: StructSize: %04x - CRC: %08x - "
           "Flags: %02x - BootOptionOverride: %02x\n",
           BoardBootOverride.StructSize, BoardBootOverride.Checksum,
           BoardBootOverride.Flags, BoardBootOverride.BootOptionOverride
           ));
    CRC32Array = CalculateCrc32((UINT8 *)&BoardBootOverride.Flags, sizeof(BOARD_BOOT_OVERRIDE) - OFFSET_OF(BOARD_BOOT_OVERRIDE, Flags));
    if (CRC32Array != BoardBootOverride.Checksum) {
      DEBUG((DEBUG_ERROR,"SMBusConfigLoader: Boot Override, Checksum invalid\n"));
    } else {
      BootOverride.Type = BoardBootOverride.BootOptionOverride;
      BootOverride.Port = BoardBootOverride.Port;

      Status = gRT->SetVariable(
          BOARD_BOOT_OVERRIDE_NAME,           // Variable Name
          &gEfiBoardBootOverrideVariableGuid, // Variable Guid
          (EFI_VARIABLE_BOOTSERVICE_ACCESS),  // Variable Attributes
          sizeof(BootOverride), &BootOverride);

      BOOLEAN DoOnce = BoardBootOverride.Flags & 1;
      if (DoOnce) {
        UINT8 *Byte = (UINT8 *)&BoardBootOverride;
        BoardBootOverride.Flags = 0;
        BoardBootOverride.BootOptionOverride = 0;
        BoardBootOverride.Checksum = CalculateCrc32(
            (UINT8 *)&BoardBootOverride.Flags,
            sizeof(BOARD_BOOT_OVERRIDE) - OFFSET_OF(BOARD_BOOT_OVERRIDE, Flags));
        for (INTN Index = OFFSET_OF(BOARD_BOOT_OVERRIDE, Checksum); Index < sizeof(BOARD_BOOT_OVERRIDE); Index++) {
          Status = WriteToEEPROM(*(Byte + Index), BOARD_BOOT_OVERRIDE_OFFSET + Index);
          if (EFI_ERROR(Status))
            DEBUG((DEBUG_ERROR, "SMBusConfigLoader: error writing boot override flags\n"));
        }
      }
    }
  }
  // Disable boot timeout if boot menu is disabled
  if (BoardSettings.MenuDisabled == 1) {
    PcdStatus = PcdSet16S (PcdPlatformBootTimeOut, 0);
    ASSERT_RETURN_ERROR (PcdStatus);
    DEBUG((DEBUG_INFO, "Boot menu disabled\n"));
  }

  // Set SecureBootEnable. Only affects SecureBootSetupDxe.
  Status = gRT->SetVariable (EFI_SECURE_BOOT_ENABLE_NAME, 
           &gEfiSecureBootEnableDisableGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof BoardSettings.SecureBoot, &BoardSettings.SecureBoot);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "SMBusConfigLoader: Failed to set SecureBootEnable: %x\n", Status));
  }

  // Set SecureBoot. Only affects code outside of SecureBootSetupDxe.
  Status = gRT->SetVariable (EFI_SECURE_BOOT_MODE_NAME, 
           &gEfiGlobalVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof BoardSettings.SecureBoot, &BoardSettings.SecureBoot);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "SMBusConfigLoader: Failed to set SecureBoot: %x\n", Status));
  }

  UINT8 SetupMode;
  // EFI_SETUP_MODE_NAME must be valid if EFI_SECURE_BOOT_MODE_NAME exists
  // Force setup mode to enroll default keys
  SetupMode = 1;
  Status = gRT->SetVariable (EFI_SETUP_MODE_NAME, 
           &gEfiGlobalVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof SetupMode, &SetupMode);

  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "SMBusConfigLoader: Failed to set SetupMode: %x\n", Status));
  }

  // Restore I2C_EN Bit
  PciWrite32(BaseAddress + HOSTC, HostCBackup);

  // Save data into UEFI Variable for later use
  Status = gRT->SetVariable(
                  BOARD_SETTINGS_NAME,               // Variable Name
                  &gEfiBoardSettingsVariableGuid, // Variable Guid
                  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS),// Variable Attributes
                  sizeof(BOARD_SETTINGS), 
                  &BoardSettings
                  );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "SMBusConfigLoader: Failed to save BoardSettings %x\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}
