/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


**/

#include <Library/SpiFlash.H>

#define FLASH_SIZE  0x300000
#define FLASH_DEVICE_BASE_ADDRESS (0xFFFFFFFF-FLASH_SIZE+1)

//
// Serial Flash device initialization data table provided to the
// Intel(R) SPI Host Controller Compatibility Interface.
//
SPI_INIT_TABLE  mInitTable[] = {
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25QXX,     // DeviceId 0
  SF_DEVICE_ID1_W25Q64,     // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25Q64_SIZE >= FLASH_SIZE) ? WINBOND_W25Q64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_ATMEL,       // VendorId
  SF_DEVICE_ID0_AT25DF321A,  // DeviceId 0
  SF_DEVICE_ID1_AT25DF321A, // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable      },     // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (32KB)
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((ATMEL_AT25DF321A_SIZE >= FLASH_SIZE) ? ATMEL_AT25DF321A_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_ATMEL,       // VendorId
  SF_DEVICE_ID0_AT26DF321,  // DeviceId 0
  SF_DEVICE_ID1_AT26DF321,  // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable      },     // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (32KB)
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((ATMEL_AT26DF321_SIZE >= FLASH_SIZE) ? ATMEL_AT26DF321_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_ATMEL,       // VendorId
  SF_DEVICE_ID0_AT25DF641,  // DeviceId 0
  SF_DEVICE_ID1_AT25DF641,  // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable      },     // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (32KB)
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((ATMEL_AT25DF641_SIZE >= FLASH_SIZE) ? ATMEL_AT25DF641_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25QXX,     // DeviceId 0
  SF_DEVICE_ID1_W25Q16,     // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25Q16_SIZE >= FLASH_SIZE) ? WINBOND_W25Q16_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25QXX,     // DeviceId 0
  SF_DEVICE_ID1_W25Q32,     // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register.
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25Q32_SIZE >= FLASH_SIZE) ? WINBOND_W25Q32_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25XXX,     // DeviceId 0
  SF_DEVICE_ID1_W25X32,     // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25X32_SIZE >= FLASH_SIZE) ? WINBOND_W25X32_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25XXX,     // DeviceId 0
  SF_DEVICE_ID1_W25X64,     // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData      },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},    // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25X64_SIZE >= FLASH_SIZE) ? WINBOND_W25X64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_WINBOND,     // VendorId
  SF_DEVICE_ID0_W25QXX,     // DeviceId 0
  SF_DEVICE_ID1_W25Q128,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((WINBOND_W25Q128_SIZE >= FLASH_SIZE) ? WINBOND_W25Q128_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_MACRONIX,    // VendorId
  SF_DEVICE_ID0_MX25LXX,    // DeviceId 0
  SF_DEVICE_ID1_MX25L16,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1:  Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((MACRONIX_MX25L16_SIZE >= FLASH_SIZE) ? MACRONIX_MX25L16_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_MACRONIX,    // VendorId
  SF_DEVICE_ID0_MX25LXX,    // DeviceId 0
  SF_DEVICE_ID1_MX25L32,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1:  Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((MACRONIX_MX25L32_SIZE >= FLASH_SIZE) ? MACRONIX_MX25L32_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_MACRONIX,    // VendorId
  SF_DEVICE_ID0_MX25LXX,    // DeviceId 0
  SF_DEVICE_ID1_MX25L64,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1:  Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((MACRONIX_MX25L64_SIZE >= FLASH_SIZE) ? MACRONIX_MX25L64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_MACRONIX,    // VendorId
  SF_DEVICE_ID0_MX25LXX,    // DeviceId 0
  SF_DEVICE_ID1_MX25L128,   // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1:  Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,           SF_INST_SFDP,           EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},    // Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((MACRONIX_MX25L128_SIZE >= FLASH_SIZE) ? MACRONIX_MX25L128_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_MACRONIX,	  // VendorId
  SF_DEVICE_ID0_MX25UXX,	  // DeviceId 0
  SF_DEVICE_ID1_MX25U6435F,   // DeviceId 1
  {
    SF_INST_WREN, 		      // Prefix Opcode 0: Write Enable
    SF_INST_EWSR			  // Prefix Opcode 1:  Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr, 	SF_INST_JEDEC_READ_ID,	EnumSpiCycle50MHz, EnumSpiOperationJedecId		},		// Opcode 0: Read ID
    {EnumSpiOpcodeRead,			SF_INST_READ,			EnumSpiCycle33MHz, EnumSpiOperationReadData 	 }, 	 // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr, 	SF_INST_RDSR,			EnumSpiCycle50MHz, EnumSpiOperationReadStatus	 }, 	 // Opcode 2: Read Status Register
    {EnumSpiOpcodeRead,			SF_INST_SFDP,			EnumSpiCycle50MHz, EnumSpiOperationDiscoveryParameters},	// Opcode 3: Serial Flash Discovery Parameters
    {EnumSpiOpcodeWrite,		SF_INST_SERASE, 		EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },	  // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,		SF_INST_64KB_ERASE, 	EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },	   // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,		SF_INST_PROG,			EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte}, 	// Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,	SF_INST_WRSR,			EnumSpiCycle50MHz, EnumSpiOperationWriteStatus	  },	  // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((MACRONIX_MX25U64_SIZE >= FLASH_SIZE) ? MACRONIX_MX25U64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_SST,         // VendorId
  SF_DEVICE_ID0_SST25VF0XXX,// DeviceId 0
  SF_DEVICE_ID1_SST25VF016B,// DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle20MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable      },     // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (32KB)
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((SST_SST25VF016B_SIZE >= FLASH_SIZE) ? SST_SST25VF016B_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_SST,         // VendorId
  SF_DEVICE_ID0_SST25VF0XXX,// DeviceId 0
  SF_DEVICE_ID1_SST25VF064C,// DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_EWSR            // Prefix Opcode 1: Enable Write Status Register
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId           },     // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle20MHz, EnumSpiOperationReadData          },     // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus        },     // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable      },     // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte     },     // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte    },     // Opcode 5: Block Erase (32KB)
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus       },     // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((SST_SST25VF064C_SIZE >= FLASH_SIZE) ? SST_SST25VF064C_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  	//
  	// Minnow2 SPI type
  	//
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_N25Q064,    // DeviceId 0
  SF_DEVICE_ID1_N25Q064,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle20MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle20MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle20MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle20MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle20MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle20MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle20MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle20MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_N25Q064_SIZE >= FLASH_SIZE) ? NUMONYX_N25Q064_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_M25PXXX,    // DeviceId 0
  SF_DEVICE_ID1_M25PX16,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_M25PX16_SIZE >= FLASH_SIZE) ? NUMONYX_M25PX16_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_N25QXXX,    // DeviceId 0
  SF_DEVICE_ID1_N25Q032,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },        // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },        // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },        // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},   // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_N25Q032_SIZE >= FLASH_SIZE) ? NUMONYX_N25Q032_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_M25PXXX,    // DeviceId 0
  SF_DEVICE_ID1_M25PX32,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_M25PX32_SIZE >= FLASH_SIZE) ? NUMONYX_M25PX32_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_M25PXXX,    // DeviceId 0
  SF_DEVICE_ID1_M25PX64,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_M25PX64_SIZE >= FLASH_SIZE) ? NUMONYX_M25PX64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_NUMONYX,     // VendorId
  SF_DEVICE_ID0_N25QXXX,    // DeviceId 0
  SF_DEVICE_ID1_N25Q128,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((NUMONYX_N25Q128_SIZE >= FLASH_SIZE) ? NUMONYX_N25Q128_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_EON,         // VendorId
  SF_DEVICE_ID0_EN25QXX,    // DeviceId 0
  SF_DEVICE_ID1_EN25Q16,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((EON_EN25Q16_SIZE >= FLASH_SIZE) ? EON_EN25Q16_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_EON,         // VendorId
  SF_DEVICE_ID0_EN25QXX,    // DeviceId 0
  SF_DEVICE_ID1_EN25Q32,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle33MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((EON_EN25Q32_SIZE >= FLASH_SIZE) ? EON_EN25Q32_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_EON,         // VendorId
  SF_DEVICE_ID0_EN25QXX,    // DeviceId 0
  SF_DEVICE_ID1_EN25Q64,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR).
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((EON_EN25Q64_SIZE >= FLASH_SIZE) ? EON_EN25Q64_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_EON,         // VendorId
  SF_DEVICE_ID0_EN25QXX,    // DeviceId 0
  SF_DEVICE_ID1_EN25Q128,   // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((EON_EN25Q128_SIZE >= FLASH_SIZE) ? EON_EN25Q128_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  },
  {
  SF_VENDOR_ID_AMIC,        // VendorId
  SF_DEVICE_ID0_A25L016,    // DeviceId 0
  SF_DEVICE_ID1_A25L016,    // DeviceId 1
  {
    SF_INST_WREN,           // Prefix Opcode 0: Write Enable
    SF_INST_WREN            // Prefix Opcode 1: Write Enable (this part doesn't support EWSR)
  },
  {
    {EnumSpiOpcodeReadNoAddr,     SF_INST_JEDEC_READ_ID,  EnumSpiCycle50MHz, EnumSpiOperationJedecId      },      // Opcode 0: Read ID
    {EnumSpiOpcodeRead,           SF_INST_READ,           EnumSpiCycle50MHz, EnumSpiOperationReadData      },      // Opcode 1: Read
    {EnumSpiOpcodeReadNoAddr,     SF_INST_RDSR,           EnumSpiCycle50MHz, EnumSpiOperationReadStatus    },      // Opcode 2: Read Status Register
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRDI,           EnumSpiCycle50MHz, EnumSpiOperationWriteDisable    },      // Opcode 3: Write Disable
    {EnumSpiOpcodeWrite,          SF_INST_SERASE,         EnumSpiCycle50MHz, EnumSpiOperationErase_4K_Byte  },      // Opcode 4: Sector Erase (4KB)
    {EnumSpiOpcodeWrite,          SF_INST_64KB_ERASE,     EnumSpiCycle50MHz, EnumSpiOperationErase_64K_Byte  },      // Opcode 5: Block Erase (64KB
    {EnumSpiOpcodeWrite,          SF_INST_PROG,           EnumSpiCycle50MHz, EnumSpiOperationProgramData_1_Byte},     // Opcode 6: Byte Program
    {EnumSpiOpcodeWriteNoAddr,    SF_INST_WRSR,           EnumSpiCycle50MHz, EnumSpiOperationWriteStatus    },      // Opcode 7: Write Status Register
  },

  //
  // The offset of the start of the BIOS image in flash. This value is platform specific
  // and depends on the system flash map. If BIOS size is bigger than flash return -1.
  //
  ((AMIC_A25L16_SIZE >= FLASH_SIZE) ? AMIC_A25L16_SIZE - FLASH_SIZE : (UINTN) (-1)),

  //
  // The size of the BIOS image in flash. This value is platform specific and depends on the system flash map.
  //
  FLASH_SIZE
  }
};
