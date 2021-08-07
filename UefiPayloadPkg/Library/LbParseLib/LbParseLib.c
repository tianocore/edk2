/** @file
  This library will parse the linuxboot table in memory and extract those required
  information.

  Copyright (c) 2021, the u-root Authors. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BlParseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Linuxboot.h>
#include <Uefi/UefiBaseType.h>

// Retrieve UefiPayloadConfig from Linuxboot's uefiboot
UefiPayloadConfig*
GetUefiPayLoadConfig()
{
  UefiPayloadConfig *Config;
  Config = (UefiPayloadConfig*)(UINTN)(PcdGet32(PcdPayloadFdMemBase) - SIZE_64KB);
  if (Config->Version != UEFI_PAYLOAD_CONFIG_VERSION) {
    DEBUG((DEBUG_ERROR, "Expect payload Config version: %d, but get %d\n",
           UEFI_PAYLOAD_CONFIG_VERSION, Config->Version));
    CpuDeadLoop ();
  }
  return Config;
}

// Align the address and add memory rang to MemInfoCallback
VOID
AddMemoryRange (
  IN BL_MEM_INFO_CALLBACK MemInfoCallback,
  IN UINTN start,
  IN UINTN end,
  IN int type
  ) {
  MEMROY_MAP_ENTRY MemoryMap;
  UINTN AlignedStart;
  UINTN AlignedEnd;
  AlignedStart = ALIGN_VALUE(start, SIZE_4KB);
  AlignedEnd = ALIGN_VALUE(end, SIZE_4KB);
  // Conservative adjustment on Memory map. This should happen when booting from
  // non UEFI bios and it may report a memory region less than 4KB.
  if (AlignedStart > start && type != LINUXBOOT_MEM_RAM) {
    AlignedStart -= SIZE_4KB;
  }
  if (AlignedEnd > end + 1 && type == LINUXBOOT_MEM_RAM) {
    AlignedEnd -= SIZE_4KB;
  }
  MemoryMap.Base = AlignedStart;
  MemoryMap.Size = AlignedEnd - AlignedStart;
  MemoryMap.Type = type;
  MemoryMap.Flag = 0;
  MemInfoCallback(&MemoryMap, NULL);
}

/**
  Acquire the memory information from the linuxboot table in memory.

  @param  MemInfoCallback     The callback routine
  @param  Params              Pointer to the callback routine parameter

  @retval RETURN_SUCCESS     Successfully find out the memory information.
  @retval RETURN_NOT_FOUND   Failed to find the memory information.

**/
RETURN_STATUS
EFIAPI
ParseMemoryInfo (
  IN BL_MEM_INFO_CALLBACK MemInfoCallback,
  IN VOID* Params
  ) {
  UefiPayloadConfig *Config;
  MemoryMapEntry* entry;
  int Index;

  Config = GetUefiPayLoadConfig();

  DEBUG((DEBUG_INFO, "MemoryMap #entries: %d\n", Config->NumMemoryMapEntries));

  entry = &Config->MemoryMapEntries[0];
  for (Index = 0; Index < Config->NumMemoryMapEntries; Index++) {
    DEBUG((DEBUG_INFO, "Start: 0x%lx End: 0x%lx Type:%d\n", entry->Start,
           entry->End, entry->Type));
    AddMemoryRange(MemInfoCallback, entry->Start, entry->End, entry->Type);
    entry++;
  }
  return RETURN_SUCCESS;
}

/**
  Acquire acpi table and smbios table from linuxboot

  @param  SystemTableInfo          Pointer to the system table info

  @retval RETURN_SUCCESS            Successfully find out the tables.
  @retval RETURN_NOT_FOUND          Failed to find the tables.

**/
RETURN_STATUS
EFIAPI
ParseSystemTable (
  OUT SYSTEM_TABLE_INFO* SystemTableInfo
  ) {
  UefiPayloadConfig *Config;

  Config = GetUefiPayLoadConfig();
  SystemTableInfo->AcpiTableBase = Config->AcpiBase;
  SystemTableInfo->AcpiTableSize = Config->AcpiSize;

  SystemTableInfo->SmbiosTableBase = Config->SmbiosBase;
  SystemTableInfo->SmbiosTableSize = Config->SmbiosSize;

  return RETURN_SUCCESS;
}

/**
  Find the serial port information

  @param  SERIAL_PORT_INFO   Pointer to serial port info structure

  @retval RETURN_SUCCESS     Successfully find the serial port information.
  @retval RETURN_NOT_FOUND   Failed to find the serial port information .

**/
RETURN_STATUS
EFIAPI
ParseSerialInfo (
  OUT SERIAL_PORT_INFO* SerialPortInfo
  ) {
  UefiPayloadConfig *Config;
  Config = GetUefiPayLoadConfig();

  SerialPortInfo->BaseAddr = Config->SerialConfig.BaseAddr;
  SerialPortInfo->RegWidth = Config->SerialConfig.RegWidth;
  SerialPortInfo->Type = Config->SerialConfig.Type;
  SerialPortInfo->Baud = Config->SerialConfig.Baud;
  SerialPortInfo->InputHertz = Config->SerialConfig.InputHertz;
  SerialPortInfo->UartPciAddr = Config->SerialConfig.UartPciAddr;

  return RETURN_SUCCESS;
}

/**
  Find the video frame buffer information

  @param  GfxInfo             Pointer to the EFI_PEI_GRAPHICS_INFO_HOB structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer
information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information .

**/
RETURN_STATUS
EFIAPI
ParseGfxInfo (
  OUT EFI_PEI_GRAPHICS_INFO_HOB* GfxInfo
  ) {
  // Not supported
  return RETURN_NOT_FOUND;
}

/**
  Find the video frame buffer device information

  @param  GfxDeviceInfo      Pointer to the EFI_PEI_GRAPHICS_DEVICE_INFO_HOB
structure

  @retval RETURN_SUCCESS     Successfully find the video frame buffer
information.
  @retval RETURN_NOT_FOUND   Failed to find the video frame buffer information.

**/
RETURN_STATUS
EFIAPI
ParseGfxDeviceInfo (
  OUT EFI_PEI_GRAPHICS_DEVICE_INFO_HOB* GfxDeviceInfo
  ) {
  return RETURN_NOT_FOUND;
}
