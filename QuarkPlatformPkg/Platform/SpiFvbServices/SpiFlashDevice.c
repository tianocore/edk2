/** @file
Initializes Platform Specific Drivers.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "SpiFlashDevice.h"

#define FLASH_SIZE  (FixedPcdGet32 (PcdFlashAreaSize))

SPI_INIT_TABLE  mSpiInitTable[] = {
  //
  // Macronix 32Mbit part
  //
  {
    SPI_MX25L3205_ID1,
    SPI_MX25L3205_ID2,
    SPI_MX25L3205_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle33MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle33MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle33MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle33MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle20MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_BLOCK_ERASE, EnumSpiCycle33MHz, EnumSpiOperationErase_64K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle33MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle33MHz, EnumSpiOperationFullChipErase}
    },
    0x400000 - FLASH_SIZE, // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Winbond 32Mbit part
  //
  {
    SPI_W25X32_ID1,
    SF_DEVICE_ID0_W25QXX,
    SF_DEVICE_ID1_W25Q32,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x400000 - FLASH_SIZE, // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Winbond 32Mbit part
  //
  {
    SPI_W25X32_ID1,
    SPI_W25X32_ID2,
    SPI_W25X32_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle33MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle33MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle33MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle33MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle33MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle33MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle33MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle33MHz, EnumSpiOperationFullChipErase}
    },
    0x400000 - FLASH_SIZE, // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Atmel 32Mbit part
  //
  {
    SPI_AT26DF321_ID1,
    SPI_AT26DF321_ID2,  // issue: byte 2 identifies family/density for Atmel
    SPI_AT26DF321_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle33MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle33MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle33MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle33MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle33MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_BLOCK_ERASE, EnumSpiCycle33MHz, EnumSpiOperationErase_64K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle33MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle33MHz, EnumSpiOperationFullChipErase}
    },
    0x400000 - FLASH_SIZE, // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },

  //
  // Intel 32Mbit part bottom boot
  //
  {
    SPI_QH25F320_ID1,
    SPI_QH25F320_ID2,
    SPI_QH25F320_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_ENABLE
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle33MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle33MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle33MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle33MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle33MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_BLOCK_ERASE, EnumSpiCycle33MHz, EnumSpiOperationErase_64K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle33MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle33MHz, EnumSpiOperationFullChipErase}
    },
    0,           // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // SST 64Mbit part
  //
  {
    SPI_SST25VF080B_ID1,      // VendorId
    SF_DEVICE_ID0_25VF064C,   // DeviceId 0
    SF_DEVICE_ID1_25VF064C,   // DeviceId 1
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // NUMONYX 64Mbit part
  //
  {
    SF_VENDOR_ID_NUMONYX,     // VendorId
    SF_DEVICE_ID0_M25PX64,    // DeviceId 0
    SF_DEVICE_ID1_M25PX64,    // DeviceId 1
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Atmel 64Mbit part
  //
  {
    SF_VENDOR_ID_ATMEL,       // VendorId
    SF_DEVICE_ID0_AT25DF641,  // DeviceId 0
    SF_DEVICE_ID1_AT25DF641,  // DeviceId 1
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },

  //
  // Spansion 64Mbit part
  //
  {
    SF_VENDOR_ID_SPANSION,       // VendorId
    SF_DEVICE_ID0_S25FL064K,  // DeviceId 0
    SF_DEVICE_ID1_S25FL064K,  // DeviceId 1
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },

  //
  // Macronix 64Mbit part bottom boot
  //
  {
    SF_VENDOR_ID_MX,          // VendorId
    SF_DEVICE_ID0_25L6405D,   // DeviceId 0
    SF_DEVICE_ID1_25L6405D,   // DeviceId 1
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_BLOCK_ERASE, EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Winbond 64Mbit part bottom boot
  //
  {
    SPI_W25X64_ID1,
    SF_DEVICE_ID0_W25QXX,
    SF_DEVICE_ID1_W25Q64,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Winbond 64Mbit part bottom boot
  //
  {
    SPI_W25X64_ID1,
    SPI_W25X64_ID2,
    SPI_W25X64_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle50MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle50MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle50MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle50MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_ERASE,       EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle50MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle50MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  },
  //
  // Intel 64Mbit part bottom boot
  //
  {
    SPI_QH25F640_ID1,
    SPI_QH25F640_ID2,
    SPI_QH25F640_ID3,
    {
      SPI_COMMAND_WRITE_ENABLE,
      SPI_COMMAND_WRITE_S_EN
    },
    {
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_JEDEC_ID,    EnumSpiCycle33MHz, EnumSpiOperationJedecId},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ_ID,     EnumSpiCycle33MHz, EnumSpiOperationOther},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_WRITE_S,     EnumSpiCycle33MHz, EnumSpiOperationWriteStatus},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_WRITE,       EnumSpiCycle33MHz, EnumSpiOperationProgramData_1_Byte},
      {EnumSpiOpcodeRead,        SPI_COMMAND_READ,        EnumSpiCycle33MHz, EnumSpiOperationReadData},
      {EnumSpiOpcodeWrite,       SPI_COMMAND_BLOCK_ERASE, EnumSpiCycle33MHz, EnumSpiOperationErase_64K_Byte},
      {EnumSpiOpcodeReadNoAddr,  SPI_COMMAND_READ_S,      EnumSpiCycle33MHz, EnumSpiOperationReadStatus},
      {EnumSpiOpcodeWriteNoAddr, SPI_COMMAND_CHIP_ERASE,  EnumSpiCycle33MHz, EnumSpiOperationFullChipErase}
    },
    0x800000 - FLASH_SIZE,          // BIOS Start Offset
    FLASH_SIZE   // BIOS image size in flash
  }
};
