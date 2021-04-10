/** @file
  Main file for Pci shell Debug1 function.

  Copyright (c) 2005 - 2021, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/PciRootBridgeIo.h>
#include <Library/ShellLib.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include "Pci.h"

//
// Printable strings for Pci class code
//
typedef struct {
  CHAR16  *BaseClass; // Pointer to the PCI base class string
  CHAR16  *SubClass;  // Pointer to the PCI sub class string
  CHAR16  *PIFClass;  // Pointer to the PCI programming interface string
} PCI_CLASS_STRINGS;

//
// a structure holding a single entry, which also points to its lower level
// class
//
typedef struct PCI_CLASS_ENTRY_TAG {
  UINT8                       Code;             // Class, subclass or I/F code
  CHAR16                      *DescText;        // Description string
  struct PCI_CLASS_ENTRY_TAG  *LowerLevelClass; // Subclass or I/F if any
} PCI_CLASS_ENTRY;

//
// Declarations of entries which contain printable strings for class codes
// in PCI configuration space
//
PCI_CLASS_ENTRY PCIBlankEntry[];
PCI_CLASS_ENTRY PCISubClass_00[];
PCI_CLASS_ENTRY PCISubClass_01[];
PCI_CLASS_ENTRY PCISubClass_02[];
PCI_CLASS_ENTRY PCISubClass_03[];
PCI_CLASS_ENTRY PCISubClass_04[];
PCI_CLASS_ENTRY PCISubClass_05[];
PCI_CLASS_ENTRY PCISubClass_06[];
PCI_CLASS_ENTRY PCISubClass_07[];
PCI_CLASS_ENTRY PCISubClass_08[];
PCI_CLASS_ENTRY PCISubClass_09[];
PCI_CLASS_ENTRY PCISubClass_0a[];
PCI_CLASS_ENTRY PCISubClass_0b[];
PCI_CLASS_ENTRY PCISubClass_0c[];
PCI_CLASS_ENTRY PCISubClass_0d[];
PCI_CLASS_ENTRY PCISubClass_0e[];
PCI_CLASS_ENTRY PCISubClass_0f[];
PCI_CLASS_ENTRY PCISubClass_10[];
PCI_CLASS_ENTRY PCISubClass_11[];
PCI_CLASS_ENTRY PCISubClass_12[];
PCI_CLASS_ENTRY PCISubClass_13[];
PCI_CLASS_ENTRY PCIPIFClass_0100[];
PCI_CLASS_ENTRY PCIPIFClass_0101[];
PCI_CLASS_ENTRY PCIPIFClass_0105[];
PCI_CLASS_ENTRY PCIPIFClass_0106[];
PCI_CLASS_ENTRY PCIPIFClass_0107[];
PCI_CLASS_ENTRY PCIPIFClass_0108[];
PCI_CLASS_ENTRY PCIPIFClass_0109[];
PCI_CLASS_ENTRY PCIPIFClass_0300[];
PCI_CLASS_ENTRY PCIPIFClass_0604[];
PCI_CLASS_ENTRY PCIPIFClass_0609[];
PCI_CLASS_ENTRY PCIPIFClass_060b[];
PCI_CLASS_ENTRY PCIPIFClass_0700[];
PCI_CLASS_ENTRY PCIPIFClass_0701[];
PCI_CLASS_ENTRY PCIPIFClass_0703[];
PCI_CLASS_ENTRY PCIPIFClass_0800[];
PCI_CLASS_ENTRY PCIPIFClass_0801[];
PCI_CLASS_ENTRY PCIPIFClass_0802[];
PCI_CLASS_ENTRY PCIPIFClass_0803[];
PCI_CLASS_ENTRY PCIPIFClass_0904[];
PCI_CLASS_ENTRY PCIPIFClass_0c00[];
PCI_CLASS_ENTRY PCIPIFClass_0c03[];
PCI_CLASS_ENTRY PCIPIFClass_0c07[];
PCI_CLASS_ENTRY PCIPIFClass_0d01[];
PCI_CLASS_ENTRY PCIPIFClass_0e00[];

//
// Base class strings entries
//
PCI_CLASS_ENTRY gClassStringList[] = {
  {
    0x00,
    L"Pre 2.0 device",
    PCISubClass_00
  },
  {
    0x01,
    L"Mass Storage Controller",
    PCISubClass_01
  },
  {
    0x02,
    L"Network Controller",
    PCISubClass_02
  },
  {
    0x03,
    L"Display Controller",
    PCISubClass_03
  },
  {
    0x04,
    L"Multimedia Device",
    PCISubClass_04
  },
  {
    0x05,
    L"Memory Controller",
    PCISubClass_05
  },
  {
    0x06,
    L"Bridge Device",
    PCISubClass_06
  },
  {
    0x07,
    L"Simple Communications Controllers",
    PCISubClass_07
  },
  {
    0x08,
    L"Base System Peripherals",
    PCISubClass_08
  },
  {
    0x09,
    L"Input Devices",
    PCISubClass_09
  },
  {
    0x0a,
    L"Docking Stations",
    PCISubClass_0a
  },
  {
    0x0b,
    L"Processors",
    PCISubClass_0b
  },
  {
    0x0c,
    L"Serial Bus Controllers",
    PCISubClass_0c
  },
  {
    0x0d,
    L"Wireless Controllers",
    PCISubClass_0d
  },
  {
    0x0e,
    L"Intelligent IO Controllers",
    PCISubClass_0e
  },
  {
    0x0f,
    L"Satellite Communications Controllers",
    PCISubClass_0f
  },
  {
    0x10,
    L"Encryption/Decryption Controllers",
    PCISubClass_10
  },
  {
    0x11,
    L"Data Acquisition & Signal Processing Controllers",
    PCISubClass_11
  },
  {
    0x12,
    L"Processing Accelerators",
    PCISubClass_12
  },
  {
    0x13,
    L"Non-Essential Instrumentation",
    PCISubClass_13
  },
  {
    0xff,
    L"Device does not fit in any defined classes",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

//
// Subclass strings entries
//
PCI_CLASS_ENTRY PCIBlankEntry[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_00[] = {
  {
    0x00,
    L"All devices other than VGA",
    PCIBlankEntry
  },
  {
    0x01,
    L"VGA-compatible devices",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_01[] = {
  {
    0x00,
    L"SCSI",
    PCIPIFClass_0100
  },
  {
    0x01,
    L"IDE controller",
    PCIPIFClass_0101
  },
  {
    0x02,
    L"Floppy disk controller",
    PCIBlankEntry
  },
  {
    0x03,
    L"IPI controller",
    PCIBlankEntry
  },
  {
    0x04,
    L"RAID controller",
    PCIBlankEntry
  },
  {
    0x05,
    L"ATA controller with ADMA interface",
    PCIPIFClass_0105
  },
  {
    0x06,
    L"Serial ATA controller",
    PCIPIFClass_0106
  },
  {
    0x07,
    L"Serial Attached SCSI (SAS) controller ",
    PCIPIFClass_0107
  },
  {
    0x08,
    L"Non-volatile memory subsystem",
    PCIPIFClass_0108
  },
  {
    0x09,
    L"Universal Flash Storage (UFS) controller ",
    PCIPIFClass_0109
  },
  {
    0x80,
    L"Other mass storage controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_02[] = {
  {
    0x00,
    L"Ethernet controller",
    PCIBlankEntry
  },
  {
    0x01,
    L"Token ring controller",
    PCIBlankEntry
  },
  {
    0x02,
    L"FDDI controller",
    PCIBlankEntry
  },
  {
    0x03,
    L"ATM controller",
    PCIBlankEntry
  },
  {
    0x04,
    L"ISDN controller",
    PCIBlankEntry
  },
  {
    0x05,
    L"WorldFip controller",
    PCIBlankEntry
  },
  {
    0x06,
    L"PICMG 2.14 Multi Computing",
    PCIBlankEntry
  },
  {
    0x07,
    L"InfiniBand controller",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other network controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_03[] = {
  {
    0x00,
    L"VGA/8514 controller",
    PCIPIFClass_0300
  },
  {
    0x01,
    L"XGA controller",
    PCIBlankEntry
  },
  {
    0x02,
    L"3D controller",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other display controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */PCIBlankEntry
  }
};

PCI_CLASS_ENTRY PCISubClass_04[] = {
  {
    0x00,
    L"Video device",
    PCIBlankEntry
  },
  {
    0x01,
    L"Audio device",
    PCIBlankEntry
  },
  {
    0x02,
    L"Computer Telephony device",
    PCIBlankEntry
  },
  {
    0x03,
    L"Mixed mode device",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other multimedia device",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_05[] = {
  {
    0x00,
    L"RAM memory controller",
    PCIBlankEntry
  },
  {
    0x01,
    L"Flash memory controller",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other memory controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_06[] = {
  {
    0x00,
    L"Host/PCI bridge",
    PCIBlankEntry
  },
  {
    0x01,
    L"PCI/ISA bridge",
    PCIBlankEntry
  },
  {
    0x02,
    L"PCI/EISA bridge",
    PCIBlankEntry
  },
  {
    0x03,
    L"PCI/Micro Channel bridge",
    PCIBlankEntry
  },
  {
    0x04,
    L"PCI/PCI bridge",
    PCIPIFClass_0604
  },
  {
    0x05,
    L"PCI/PCMCIA bridge",
    PCIBlankEntry
  },
  {
    0x06,
    L"NuBus bridge",
    PCIBlankEntry
  },
  {
    0x07,
    L"CardBus bridge",
    PCIBlankEntry
  },
  {
    0x08,
    L"RACEway bridge",
    PCIBlankEntry
  },
  {
    0x09,
    L"Semi-transparent PCI-to-PCI bridge",
    PCIPIFClass_0609
  },
  {
    0x0A,
    L"InfiniBand-to-PCI host bridge",
    PCIBlankEntry
  },
  {
    0x0B,
    L"Advanced Switching to PCI host bridge",
    PCIPIFClass_060b
  },
  {
    0x80,
    L"Other bridge type",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_07[] = {
  {
    0x00,
    L"Serial controller",
    PCIPIFClass_0700
  },
  {
    0x01,
    L"Parallel port",
    PCIPIFClass_0701
  },
  {
    0x02,
    L"Multiport serial controller",
    PCIBlankEntry
  },
  {
    0x03,
    L"Modem",
    PCIPIFClass_0703
  },
  {
    0x04,
    L"GPIB (IEEE 488.1/2) controller",
    PCIBlankEntry
  },
  {
    0x05,
    L"Smart Card",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other communication device",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_08[] = {
  {
    0x00,
    L"PIC",
    PCIPIFClass_0800
  },
  {
    0x01,
    L"DMA controller",
    PCIPIFClass_0801
  },
  {
    0x02,
    L"System timer",
    PCIPIFClass_0802
  },
  {
    0x03,
    L"RTC controller",
    PCIPIFClass_0803
  },
  {
    0x04,
    L"Generic PCI Hot-Plug controller",
    PCIBlankEntry
  },
  {
    0x05,
    L"SD Host controller",
    PCIBlankEntry
  },
  {
    0x06,
    L"IOMMU",
    PCIBlankEntry
  },
  {
    0x07,
    L"Root Complex Event Collector",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other system peripheral",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_09[] = {
  {
    0x00,
    L"Keyboard controller",
    PCIBlankEntry
  },
  {
    0x01,
    L"Digitizer (pen)",
    PCIBlankEntry
  },
  {
    0x02,
    L"Mouse controller",
    PCIBlankEntry
  },
  {
    0x03,
    L"Scanner controller",
    PCIBlankEntry
  },
  {
    0x04,
    L"Gameport controller",
    PCIPIFClass_0904
  },
  {
    0x80,
    L"Other input controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0a[] = {
  {
    0x00,
    L"Generic docking station",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other type of docking station",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0b[] = {
  {
    0x00,
    L"386",
    PCIBlankEntry
  },
  {
    0x01,
    L"486",
    PCIBlankEntry
  },
  {
    0x02,
    L"Pentium",
    PCIBlankEntry
  },
  {
    0x10,
    L"Alpha",
    PCIBlankEntry
  },
  {
    0x20,
    L"PowerPC",
    PCIBlankEntry
  },
  {
    0x30,
    L"MIPS",
    PCIBlankEntry
  },
  {
    0x40,
    L"Co-processor",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other processor",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0c[] = {
  {
    0x00,
    L"IEEE 1394",
    PCIPIFClass_0c00
  },
  {
    0x01,
    L"ACCESS.bus",
    PCIBlankEntry
  },
  {
    0x02,
    L"SSA",
    PCIBlankEntry
  },
  {
    0x03,
    L"USB",
    PCIPIFClass_0c03
  },
  {
    0x04,
    L"Fibre Channel",
    PCIBlankEntry
  },
  {
    0x05,
    L"System Management Bus",
    PCIBlankEntry
  },
  {
    0x06,
    L"InfiniBand",
    PCIBlankEntry
  },
  {
    0x07,
    L"IPMI",
    PCIPIFClass_0c07
  },
  {
    0x08,
    L"SERCOS Interface Standard (IEC 61491)",
    PCIBlankEntry
  },
  {
    0x09,
    L"CANbus",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other bus type",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0d[] = {
  {
    0x00,
    L"iRDA compatible controller",
    PCIBlankEntry
  },
  {
    0x01,
    L"",
    PCIPIFClass_0d01
  },
  {
    0x10,
    L"RF controller",
    PCIBlankEntry
  },
  {
    0x11,
    L"Bluetooth",
    PCIBlankEntry
  },
  {
    0x12,
    L"Broadband",
    PCIBlankEntry
  },
  {
    0x20,
    L"Ethernet (802.11a - 5 GHz)",
    PCIBlankEntry
  },
  {
    0x21,
    L"Ethernet (802.11b - 2.4 GHz)",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other type of wireless controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0e[] = {
  {
    0x00,
    L"I2O Architecture",
    PCIPIFClass_0e00
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_0f[] = {
  {
    0x01,
    L"TV",
    PCIBlankEntry
  },
  {
    0x02,
    L"Audio",
    PCIBlankEntry
  },
  {
    0x03,
    L"Voice",
    PCIBlankEntry
  },
  {
    0x04,
    L"Data",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other satellite communication controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_10[] = {
  {
    0x00,
    L"Network & computing Encrypt/Decrypt",
    PCIBlankEntry
  },
  {
    0x01,
    L"Entertainment Encrypt/Decrypt",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other Encrypt/Decrypt",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_11[] = {
  {
    0x00,
    L"DPIO modules",
    PCIBlankEntry
  },
  {
    0x01,
    L"Performance Counters",
    PCIBlankEntry
  },
  {
    0x10,
    L"Communications synchronization plus time and frequency test/measurement ",
    PCIBlankEntry
  },
  {
    0x20,
    L"Management card",
    PCIBlankEntry
  },
  {
    0x80,
    L"Other DAQ & SP controllers",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_12[] = {
  {
    0x00,
    L"Processing Accelerator",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCISubClass_13[] = {
  {
    0x00,
    L"Non-Essential Instrumentation Function",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

//
// Programming Interface entries
//
PCI_CLASS_ENTRY PCIPIFClass_0100[] = {
  {
    0x00,
    L"SCSI controller",
    PCIBlankEntry
  },
  {
    0x11,
    L"SCSI storage device SOP using PQI",
    PCIBlankEntry
  },
  {
    0x12,
    L"SCSI controller SOP using PQI",
    PCIBlankEntry
  },
  {
    0x13,
    L"SCSI storage device and controller SOP using PQI",
    PCIBlankEntry
  },
  {
    0x21,
    L"SCSI storage device SOP using NVMe",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0101[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"OM-primary",
    PCIBlankEntry
  },
  {
    0x02,
    L"PI-primary",
    PCIBlankEntry
  },
  {
    0x03,
    L"OM/PI-primary",
    PCIBlankEntry
  },
  {
    0x04,
    L"OM-secondary",
    PCIBlankEntry
  },
  {
    0x05,
    L"OM-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x06,
    L"PI-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x07,
    L"OM/PI-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x08,
    L"OM-secondary",
    PCIBlankEntry
  },
  {
    0x09,
    L"OM-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x0a,
    L"PI-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x0b,
    L"OM/PI-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x0c,
    L"OM-secondary",
    PCIBlankEntry
  },
  {
    0x0d,
    L"OM-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x0e,
    L"PI-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x0f,
    L"OM/PI-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x80,
    L"Master",
    PCIBlankEntry
  },
  {
    0x81,
    L"Master, OM-primary",
    PCIBlankEntry
  },
  {
    0x82,
    L"Master, PI-primary",
    PCIBlankEntry
  },
  {
    0x83,
    L"Master, OM/PI-primary",
    PCIBlankEntry
  },
  {
    0x84,
    L"Master, OM-secondary",
    PCIBlankEntry
  },
  {
    0x85,
    L"Master, OM-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x86,
    L"Master, PI-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x87,
    L"Master, OM/PI-primary, OM-secondary",
    PCIBlankEntry
  },
  {
    0x88,
    L"Master, OM-secondary",
    PCIBlankEntry
  },
  {
    0x89,
    L"Master, OM-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x8a,
    L"Master, PI-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x8b,
    L"Master, OM/PI-primary, PI-secondary",
    PCIBlankEntry
  },
  {
    0x8c,
    L"Master, OM-secondary",
    PCIBlankEntry
  },
  {
    0x8d,
    L"Master, OM-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x8e,
    L"Master, PI-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x8f,
    L"Master, OM/PI-primary, OM/PI-secondary",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0105[] = {
  {
    0x20,
    L"Single stepping",
    PCIBlankEntry
  },
  {
    0x30,
    L"Continuous operation",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0106[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"AHCI",
    PCIBlankEntry
  },
  {
    0x02,
    L"Serial Storage Bus",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0107[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"Obsolete",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0108[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"NVMHCI",
    PCIBlankEntry
  },
  {
    0x02,
    L"NVM Express",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0109[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"UFSHCI",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0300[] = {
  {
    0x00,
    L"VGA compatible",
    PCIBlankEntry
  },
  {
    0x01,
    L"8514 compatible",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0604[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"Subtractive decode",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0609[] = {
  {
    0x40,
    L"Primary PCI bus side facing the system host processor",
    PCIBlankEntry
  },
  {
    0x80,
    L"Secondary PCI bus side facing the system host processor",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_060b[] = {
  {
    0x00,
    L"Custom",
    PCIBlankEntry
  },
  {
    0x01,
    L"ASI-SIG Defined Portal",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0700[] = {
  {
    0x00,
    L"Generic XT-compatible",
    PCIBlankEntry
  },
  {
    0x01,
    L"16450-compatible",
    PCIBlankEntry
  },
  {
    0x02,
    L"16550-compatible",
    PCIBlankEntry
  },
  {
    0x03,
    L"16650-compatible",
    PCIBlankEntry
  },
  {
    0x04,
    L"16750-compatible",
    PCIBlankEntry
  },
  {
    0x05,
    L"16850-compatible",
    PCIBlankEntry
  },
  {
    0x06,
    L"16950-compatible",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0701[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x01,
    L"Bi-directional",
    PCIBlankEntry
  },
  {
    0x02,
    L"ECP 1.X-compliant",
    PCIBlankEntry
  },
  {
    0x03,
    L"IEEE 1284",
    PCIBlankEntry
  },
  {
    0xfe,
    L"IEEE 1284 target (not a controller)",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0703[] = {
  {
    0x00,
    L"Generic",
    PCIBlankEntry
  },
  {
    0x01,
    L"Hayes-compatible 16450",
    PCIBlankEntry
  },
  {
    0x02,
    L"Hayes-compatible 16550",
    PCIBlankEntry
  },
  {
    0x03,
    L"Hayes-compatible 16650",
    PCIBlankEntry
  },
  {
    0x04,
    L"Hayes-compatible 16750",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0800[] = {
  {
    0x00,
    L"Generic 8259",
    PCIBlankEntry
  },
  {
    0x01,
    L"ISA",
    PCIBlankEntry
  },
  {
    0x02,
    L"EISA",
    PCIBlankEntry
  },
  {
    0x10,
    L"IO APIC",
    PCIBlankEntry
  },
  {
    0x20,
    L"IO(x) APIC interrupt controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0801[] = {
  {
    0x00,
    L"Generic 8237",
    PCIBlankEntry
  },
  {
    0x01,
    L"ISA",
    PCIBlankEntry
  },
  {
    0x02,
    L"EISA",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0802[] = {
  {
    0x00,
    L"Generic 8254",
    PCIBlankEntry
  },
  {
    0x01,
    L"ISA",
    PCIBlankEntry
  },
  {
    0x02,
    L"EISA",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0803[] = {
  {
    0x00,
    L"Generic",
    PCIBlankEntry
  },
  {
    0x01,
    L"ISA",
    PCIBlankEntry
  },
  {
    0x02,
    L"EISA",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0904[] = {
  {
    0x00,
    L"Generic",
    PCIBlankEntry
  },
  {
    0x10,
    L"",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0c00[] = {
  {
    0x00,
    L"",
    PCIBlankEntry
  },
  {
    0x10,
    L"Using 1394 OpenHCI spec",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0c03[] = {
  {
    0x00,
    L"UHCI",
    PCIBlankEntry
  },
  {
    0x10,
    L"OHCI",
    PCIBlankEntry
  },
  {
    0x20,
    L"EHCI",
    PCIBlankEntry
  },
  {
    0x30,
    L"xHCI",
    PCIBlankEntry
  },
  {
    0x80,
    L"No specific programming interface",
    PCIBlankEntry
  },
  {
    0xfe,
    L"(Not Host Controller)",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0c07[] = {
  {
    0x00,
    L"SMIC",
    PCIBlankEntry
  },
  {
    0x01,
    L"Keyboard Controller Style",
    PCIBlankEntry
  },
  {
    0x02,
    L"Block Transfer",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0d01[] = {
  {
    0x00,
    L"Consumer IR controller",
    PCIBlankEntry
  },
  {
    0x10,
    L"UWB Radio controller",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};

PCI_CLASS_ENTRY PCIPIFClass_0e00[] = {
  {
    0x00,
    L"Message FIFO at offset 40h",
    PCIBlankEntry
  },
  {
    0x01,
    L"",
    PCIBlankEntry
  },
  {
    0x00,
    NULL,
    /* null string ends the list */NULL
  }
};


/**
  Generates printable Unicode strings that represent PCI device class,
  subclass and programmed I/F based on a value passed to the function.

  @param[in] ClassCode      Value representing the PCI "Class Code" register read from a
                 PCI device. The encodings are:
                     bits 23:16 - Base Class Code
                     bits 15:8  - Sub-Class Code
                     bits  7:0  - Programming Interface
  @param[in, out] ClassStrings   Pointer of PCI_CLASS_STRINGS structure, which contains
                 printable class strings corresponding to ClassCode. The
                 caller must not modify the strings that are pointed by
                 the fields in ClassStrings.
**/
VOID
PciGetClassStrings (
  IN      UINT32               ClassCode,
  IN OUT  PCI_CLASS_STRINGS    *ClassStrings
  )
{
  INTN            Index;
  UINT8           Code;
  PCI_CLASS_ENTRY *CurrentClass;

  //
  // Assume no strings found
  //
  ClassStrings->BaseClass = L"UNDEFINED";
  ClassStrings->SubClass  = L"UNDEFINED";
  ClassStrings->PIFClass  = L"UNDEFINED";

  CurrentClass = gClassStringList;
  Code = (UINT8) (ClassCode >> 16);
  Index = 0;

  //
  // Go through all entries of the base class, until the entry with a matching
  // base class code is found. If reaches an entry with a null description
  // text, the last entry is met, which means no text for the base class was
  // found, so no more action is needed.
  //
  while (Code != CurrentClass[Index].Code) {
    if (NULL == CurrentClass[Index].DescText) {
      return ;
    }

    Index++;
  }
  //
  // A base class was found. Assign description, and check if this class has
  // sub-class defined. If sub-class defined, no more action is needed,
  // otherwise, continue to find description for the sub-class code.
  //
  ClassStrings->BaseClass = CurrentClass[Index].DescText;
  if (NULL == CurrentClass[Index].LowerLevelClass) {
    return ;
  }
  //
  // find Subclass entry
  //
  CurrentClass  = CurrentClass[Index].LowerLevelClass;
  Code          = (UINT8) (ClassCode >> 8);
  Index         = 0;

  //
  // Go through all entries of the sub-class, until the entry with a matching
  // sub-class code is found. If reaches an entry with a null description
  // text, the last entry is met, which means no text for the sub-class was
  // found, so no more action is needed.
  //
  while (Code != CurrentClass[Index].Code) {
    if (NULL == CurrentClass[Index].DescText) {
      return ;
    }

    Index++;
  }
  //
  // A class was found for the sub-class code. Assign description, and check if
  // this sub-class has programming interface defined. If no, no more action is
  // needed, otherwise, continue to find description for the programming
  // interface.
  //
  ClassStrings->SubClass = CurrentClass[Index].DescText;
  if (NULL == CurrentClass[Index].LowerLevelClass) {
    return ;
  }
  //
  // Find programming interface entry
  //
  CurrentClass  = CurrentClass[Index].LowerLevelClass;
  Code          = (UINT8) ClassCode;
  Index         = 0;

  //
  // Go through all entries of the I/F entries, until the entry with a
  // matching I/F code is found. If reaches an entry with a null description
  // text, the last entry is met, which means no text was found, so no more
  // action is needed.
  //
  while (Code != CurrentClass[Index].Code) {
    if (NULL == CurrentClass[Index].DescText) {
      return ;
    }

    Index++;
  }
  //
  // A class was found for the I/F code. Assign description, done!
  //
  ClassStrings->PIFClass = CurrentClass[Index].DescText;
  return ;
}

/**
  Print strings that represent PCI device class, subclass and programmed I/F.

  @param[in] ClassCodePtr   Points to the memory which stores register Class Code in PCI
                            configuration space.
  @param[in] IncludePIF     If the printed string should include the programming I/F part
**/
VOID
PciPrintClassCode (
  IN      UINT8               *ClassCodePtr,
  IN      BOOLEAN             IncludePIF
  )
{
  UINT32            ClassCode;
  PCI_CLASS_STRINGS ClassStrings;

  ClassCode = 0;
  ClassCode |= (UINT32)ClassCodePtr[0];
  ClassCode |= (UINT32)(ClassCodePtr[1] << 8);
  ClassCode |= (UINT32)(ClassCodePtr[2] << 16);

  //
  // Get name from class code
  //
  PciGetClassStrings (ClassCode, &ClassStrings);

  if (IncludePIF) {
    //
    // Print base class, sub class, and programming inferface name
    //
    ShellPrintEx (-1, -1, L"%s - %s - %s",
      ClassStrings.BaseClass,
      ClassStrings.SubClass,
      ClassStrings.PIFClass
     );

  } else {
    //
    // Only print base class and sub class name
    //
    ShellPrintEx (-1, -1, L"%s - %s",
      ClassStrings.BaseClass,
      ClassStrings.SubClass
    );
  }
}

/**
  This function finds out the protocol which is in charge of the given
  segment, and its bus range covers the current bus number. It lookes
  each instances of RootBridgeIoProtocol handle, until the one meets the
  criteria is found.

  @param[in] HandleBuf       Buffer which holds all PCI_ROOT_BRIDIGE_IO_PROTOCOL handles.
  @param[in] HandleCount     Count of all PCI_ROOT_BRIDIGE_IO_PROTOCOL handles.
  @param[in] Segment         Segment number of device we are dealing with.
  @param[in] Bus             Bus number of device we are dealing with.
  @param[out] IoDev          Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS             The command completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
PciFindProtocolInterface (
  IN  EFI_HANDLE                            *HandleBuf,
  IN  UINTN                                 HandleCount,
  IN  UINT16                                Segment,
  IN  UINT16                                Bus,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       **IoDev
  );

/**
  This function gets the protocol interface from the given handle, and
  obtains its address space descriptors.

  @param[in] Handle          The PCI_ROOT_BRIDIGE_IO_PROTOCOL handle.
  @param[out] IoDev          Handle used to access configuration space of PCI device.
  @param[out] Descriptors    Points to the address space descriptors.

  @retval EFI_SUCCESS     The command completed successfully
**/
EFI_STATUS
PciGetProtocolAndResource (
  IN  EFI_HANDLE                            Handle,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       **IoDev,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     **Descriptors
  );

/**
  This function get the next bus range of given address space descriptors.
  It also moves the pointer backward a node, to get prepared to be called
  again.

  @param[in, out] Descriptors Points to current position of a serial of address space
                              descriptors.
  @param[out] MinBus          The lower range of bus number.
  @param[out] MaxBus          The upper range of bus number.
  @param[out] IsEnd           Meet end of the serial of descriptors.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciGetNextBusRange (
  IN OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    BOOLEAN                            *IsEnd
  );

/**
  Explain the data in PCI configuration space. The part which is common for
  PCI device and bridge is interpreted in this function. It calls other
  functions to interpret data unique for device or bridge.

  @param[in] ConfigSpace     Data in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.
**/
VOID
PciExplainPci (
  IN PCI_CONFIG_SPACE                       *ConfigSpace,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  );

/**
  Explain the device specific part of data in PCI configuration space.

  @param[in] Device          Data in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainDeviceData (
  IN PCI_DEVICE_HEADER_TYPE_REGION          *Device,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  );

/**
  Explain the bridge specific part of data in PCI configuration space.

  @param[in] Bridge          Bridge specific data region in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBridgeData (
  IN  PCI_BRIDGE_CONTROL_REGISTER           *Bridge,
  IN  UINT64                                Address,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       *IoDev
  );

/**
  Explain the Base Address Register(Bar) in PCI configuration space.

  @param[in] Bar              Points to the Base Address Register intended to interpret.
  @param[in] Command          Points to the register Command.
  @param[in] Address          Address used to access configuration space of this PCI device.
  @param[in] IoDev            Handle used to access configuration space of PCI device.
  @param[in, out] Index       The Index.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBar (
  IN UINT32                                 *Bar,
  IN UINT16                                 *Command,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev,
  IN OUT UINTN                              *Index
  );

/**
  Explain the cardbus specific part of data in PCI configuration space.

  @param[in] CardBus         CardBus specific region of PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainCardBusData (
  IN PCI_CARDBUS_CONTROL_REGISTER           *CardBus,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  );

/**
  Explain each meaningful bit of register Status. The definition of Status is
  slightly different depending on the PCI header type.

  @param[in] Status          Points to the content of register Status.
  @param[in] MainStatus      Indicates if this register is main status(not secondary
                             status).
  @param[in] HeaderType      Header type of this PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainStatus (
  IN UINT16                                 *Status,
  IN BOOLEAN                                MainStatus,
  IN PCI_HEADER_TYPE                        HeaderType
  );

/**
  Explain each meaningful bit of register Command.

  @param[in] Command         Points to the content of register Command.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainCommand (
  IN UINT16                                 *Command
  );

/**
  Explain each meaningful bit of register Bridge Control.

  @param[in] BridgeControl   Points to the content of register Bridge Control.
  @param[in] HeaderType      The headertype.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBridgeControl (
  IN UINT16                                 *BridgeControl,
  IN PCI_HEADER_TYPE                        HeaderType
  );

/**
  Locate capability register block per capability ID.

  @param[in] ConfigSpace       Data in PCI configuration space.
  @param[in] CapabilityId      The capability ID.

  @return   The offset of the register block per capability ID.
**/
UINT8
LocatePciCapability (
  IN PCI_CONFIG_SPACE   *ConfigSpace,
  IN UINT8              CapabilityId
  );

/**
  Display Pcie device structure.

  @param[in] PciExpressCap       PCI Express capability buffer.
  @param[in] ExtendedConfigSpace PCI Express extended configuration space.
  @param[in] ExtendedConfigSize  PCI Express extended configuration size.
  @param[in] ExtendedCapability  PCI Express extended capability ID to explain.
**/
VOID
PciExplainPciExpress (
  IN  PCI_CAPABILITY_PCIEXP                  *PciExpressCap,
  IN  UINT8                                  *ExtendedConfigSpace,
  IN  UINTN                                  ExtendedConfigSize,
  IN CONST UINT16                            ExtendedCapability
  );

/**
  Print out information of the capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieCapReg (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device link information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device link control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device link status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device slot information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device slot control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device slot status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device root information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device root capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

/**
  Print out information of the device root status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  );

typedef EFI_STATUS (*PCIE_EXPLAIN_FUNCTION) (IN PCI_CAPABILITY_PCIEXP *PciExpressCap);

typedef enum {
  FieldWidthUINT8,
  FieldWidthUINT16,
  FieldWidthUINT32
} PCIE_CAPREG_FIELD_WIDTH;

typedef enum {
  PcieExplainTypeCommon,
  PcieExplainTypeDevice,
  PcieExplainTypeLink,
  PcieExplainTypeSlot,
  PcieExplainTypeRoot,
  PcieExplainTypeMax
} PCIE_EXPLAIN_TYPE;

typedef struct
{
  UINT16                  Token;
  UINTN                   Offset;
  PCIE_CAPREG_FIELD_WIDTH Width;
  PCIE_EXPLAIN_FUNCTION   Func;
  PCIE_EXPLAIN_TYPE       Type;
} PCIE_EXPLAIN_STRUCT;

PCIE_EXPLAIN_STRUCT PcieExplainList[] = {
  {
    STRING_TOKEN (STR_PCIEX_CAPABILITY_CAPID),
    0x00,
    FieldWidthUINT8,
    NULL,
    PcieExplainTypeCommon
  },
  {
    STRING_TOKEN (STR_PCIEX_NEXTCAP_PTR),
    0x01,
    FieldWidthUINT8,
    NULL,
    PcieExplainTypeCommon
  },
  {
    STRING_TOKEN (STR_PCIEX_CAP_REGISTER),
    0x02,
    FieldWidthUINT16,
    ExplainPcieCapReg,
    PcieExplainTypeCommon
  },
  {
    STRING_TOKEN (STR_PCIEX_DEVICE_CAP),
    0x04,
    FieldWidthUINT32,
    ExplainPcieDeviceCap,
    PcieExplainTypeDevice
  },
  {
    STRING_TOKEN (STR_PCIEX_DEVICE_CONTROL),
    0x08,
    FieldWidthUINT16,
    ExplainPcieDeviceControl,
    PcieExplainTypeDevice
  },
  {
    STRING_TOKEN (STR_PCIEX_DEVICE_STATUS),
    0x0a,
    FieldWidthUINT16,
    ExplainPcieDeviceStatus,
    PcieExplainTypeDevice
  },
  {
    STRING_TOKEN (STR_PCIEX_LINK_CAPABILITIES),
    0x0c,
    FieldWidthUINT32,
    ExplainPcieLinkCap,
    PcieExplainTypeLink
  },
  {
    STRING_TOKEN (STR_PCIEX_LINK_CONTROL),
    0x10,
    FieldWidthUINT16,
    ExplainPcieLinkControl,
    PcieExplainTypeLink
  },
  {
    STRING_TOKEN (STR_PCIEX_LINK_STATUS),
    0x12,
    FieldWidthUINT16,
    ExplainPcieLinkStatus,
    PcieExplainTypeLink
  },
  {
    STRING_TOKEN (STR_PCIEX_SLOT_CAPABILITIES),
    0x14,
    FieldWidthUINT32,
    ExplainPcieSlotCap,
    PcieExplainTypeSlot
  },
  {
    STRING_TOKEN (STR_PCIEX_SLOT_CONTROL),
    0x18,
    FieldWidthUINT16,
    ExplainPcieSlotControl,
    PcieExplainTypeSlot
  },
  {
    STRING_TOKEN (STR_PCIEX_SLOT_STATUS),
    0x1a,
    FieldWidthUINT16,
    ExplainPcieSlotStatus,
    PcieExplainTypeSlot
  },
  {
    STRING_TOKEN (STR_PCIEX_ROOT_CONTROL),
    0x1c,
    FieldWidthUINT16,
    ExplainPcieRootControl,
    PcieExplainTypeRoot
  },
  {
    STRING_TOKEN (STR_PCIEX_RSVDP),
    0x1e,
    FieldWidthUINT16,
    ExplainPcieRootCap,
    PcieExplainTypeRoot
  },
  {
    STRING_TOKEN (STR_PCIEX_ROOT_STATUS),
    0x20,
    FieldWidthUINT32,
    ExplainPcieRootStatus,
    PcieExplainTypeRoot
  },
  {
    0,
    0,
    (PCIE_CAPREG_FIELD_WIDTH)0,
    NULL,
    PcieExplainTypeMax
  }
};

//
// Global Variables
//
PCI_CONFIG_SPACE  *mConfigSpace = NULL;
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-s", TypeValue},
  {L"-i", TypeFlag},
  {L"-ec", TypeValue},
  {NULL, TypeMax}
  };

CHAR16 *DevicePortTypeTable[] = {
  L"PCI Express Endpoint",
  L"Legacy PCI Express Endpoint",
  L"Unknown Type",
  L"Unknonw Type",
  L"Root Port of PCI Express Root Complex",
  L"Upstream Port of PCI Express Switch",
  L"Downstream Port of PCI Express Switch",
  L"PCI Express to PCI/PCI-X Bridge",
  L"PCI/PCI-X to PCI Express Bridge",
  L"Root Complex Integrated Endpoint",
  L"Root Complex Event Collector"
};

CHAR16 *L0sLatencyStrTable[] = {
  L"Less than 64ns",
  L"64ns to less than 128ns",
  L"128ns to less than 256ns",
  L"256ns to less than 512ns",
  L"512ns to less than 1us",
  L"1us to less than 2us",
  L"2us-4us",
  L"More than 4us"
};

CHAR16 *L1LatencyStrTable[] = {
  L"Less than 1us",
  L"1us to less than 2us",
  L"2us to less than 4us",
  L"4us to less than 8us",
  L"8us to less than 16us",
  L"16us to less than 32us",
  L"32us-64us",
  L"More than 64us"
};

CHAR16 *ASPMCtrlStrTable[] = {
  L"Disabled",
  L"L0s Entry Enabled",
  L"L1 Entry Enabled",
  L"L0s and L1 Entry Enabled"
};

CHAR16 *SlotPwrLmtScaleTable[] = {
  L"1.0x",
  L"0.1x",
  L"0.01x",
  L"0.001x"
};

CHAR16 *IndicatorTable[] = {
  L"Reserved",
  L"On",
  L"Blink",
  L"Off"
};


/**
  Function for 'pci' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunPci (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT16                            Segment;
  UINT16                            Bus;
  UINT16                            Device;
  UINT16                            Func;
  UINT64                            Address;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *IoDev;
  EFI_STATUS                        Status;
  PCI_DEVICE_INDEPENDENT_REGION     PciHeader;
  PCI_CONFIG_SPACE                  ConfigSpace;
  UINTN                             ScreenCount;
  UINTN                             TempColumn;
  UINTN                             ScreenSize;
  BOOLEAN                           ExplainData;
  UINTN                             Index;
  UINTN                             SizeOfHeader;
  BOOLEAN                           PrintTitle;
  UINTN                             HandleBufSize;
  EFI_HANDLE                        *HandleBuf;
  UINTN                             HandleCount;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
  UINT16                            MinBus;
  UINT16                            MaxBus;
  BOOLEAN                           IsEnd;
  LIST_ENTRY                        *Package;
  CHAR16                            *ProblemParam;
  SHELL_STATUS                      ShellStatus;
  CONST CHAR16                      *Temp;
  UINT64                            RetVal;
  UINT16                            ExtendedCapability;
  UINT8                             PcieCapabilityPtr;
  UINT8                             *ExtendedConfigSpace;
  UINTN                             ExtendedConfigSize;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  Address             = 0;
  IoDev               = NULL;
  HandleBuf           = NULL;
  Package             = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"pci", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {

    if (ShellCommandLineGetCount(Package) == 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"pci");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (ShellCommandLineGetCount(Package) > 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"pci");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }
    if (ShellCommandLineGetFlag(Package, L"-ec") && ShellCommandLineGetValue(Package, L"-ec") == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle,  L"pci", L"-ec");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }
    if (ShellCommandLineGetFlag(Package, L"-s") && ShellCommandLineGetValue(Package, L"-s") == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle,  L"pci", L"-s");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }
    //
    // Get all instances of PciRootBridgeIo. Allocate space for 1 EFI_HANDLE and
    // call LibLocateHandle(), if EFI_BUFFER_TOO_SMALL is returned, allocate enough
    // space for handles and call it again.
    //
    HandleBufSize = sizeof (EFI_HANDLE);
    HandleBuf     = (EFI_HANDLE *) AllocateZeroPool (HandleBufSize);
    if (HandleBuf == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"pci");
      ShellStatus = SHELL_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &HandleBufSize,
                  HandleBuf
                 );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      HandleBuf = ReallocatePool (sizeof (EFI_HANDLE), HandleBufSize, HandleBuf);
      if (HandleBuf == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"pci");
        ShellStatus = SHELL_OUT_OF_RESOURCES;
        goto Done;
      }

      Status = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    &HandleBufSize,
                    HandleBuf
                   );
    }

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle, L"pci");
      ShellStatus = SHELL_NOT_FOUND;
      goto Done;
    }

    HandleCount = HandleBufSize / sizeof (EFI_HANDLE);
    //
    // Argument Count == 1(no other argument): enumerate all pci functions
    //
    if (ShellCommandLineGetCount(Package) == 1) {
      gST->ConOut->QueryMode (
                    gST->ConOut,
                    gST->ConOut->Mode->Mode,
                    &TempColumn,
                    &ScreenSize
                   );

      ScreenCount = 0;
      ScreenSize -= 4;
      if ((ScreenSize & 1) == 1) {
        ScreenSize -= 1;
      }

      PrintTitle = TRUE;

      //
      // For each handle, which decides a segment and a bus number range,
      // enumerate all devices on it.
      //
      for (Index = 0; Index < HandleCount; Index++) {
        Status = PciGetProtocolAndResource (
                  HandleBuf[Index],
                  &IoDev,
                  &Descriptors
                 );
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_HANDLE_CFG_ERR), gShellDebug1HiiHandle, L"pci");
          ShellStatus = SHELL_NOT_FOUND;
          goto Done;
        }
        //
        // No document say it's impossible for a RootBridgeIo protocol handle
        // to have more than one address space descriptors, so find out every
        // bus range and for each of them do device enumeration.
        //
        while (TRUE) {
          Status = PciGetNextBusRange (&Descriptors, &MinBus, &MaxBus, &IsEnd);

          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_BUS_RANGE_ERR), gShellDebug1HiiHandle, L"pci");
            ShellStatus = SHELL_NOT_FOUND;
            goto Done;
          }

          if (IsEnd) {
            break;
          }

          for (Bus = MinBus; Bus <= MaxBus; Bus++) {
            //
            // For each devices, enumerate all functions it contains
            //
            for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
              //
              // For each function, read its configuration space and print summary
              //
              for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
                if (ShellGetExecutionBreakFlag ()) {
                  ShellStatus = SHELL_ABORTED;
                  goto Done;
                }
                Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);
                IoDev->Pci.Read (
                            IoDev,
                            EfiPciWidthUint16,
                            Address,
                            1,
                            &PciHeader.VendorId
                           );

                //
                // If VendorId = 0xffff, there does not exist a device at this
                // location. For each device, if there is any function on it,
                // there must be 1 function at Function 0. So if Func = 0, there
                // will be no more functions in the same device, so we can break
                // loop to deal with the next device.
                //
                if (PciHeader.VendorId == 0xffff && Func == 0) {
                  break;
                }

                if (PciHeader.VendorId != 0xffff) {

                  if (PrintTitle) {
                    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_TITLE), gShellDebug1HiiHandle);
                    PrintTitle = FALSE;
                  }

                  IoDev->Pci.Read (
                              IoDev,
                              EfiPciWidthUint32,
                              Address,
                              sizeof (PciHeader) / sizeof (UINT32),
                              &PciHeader
                             );

                  ShellPrintHiiEx(
                    -1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_P1), gShellDebug1HiiHandle,
                    IoDev->SegmentNumber,
                    Bus,
                    Device,
                    Func
                   );

                  PciPrintClassCode (PciHeader.ClassCode, FALSE);
                  ShellPrintHiiEx(
                    -1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_P2), gShellDebug1HiiHandle,
                    PciHeader.VendorId,
                    PciHeader.DeviceId,
                    PciHeader.ClassCode[0]
                   );

                  ScreenCount += 2;
                  if (ScreenCount >= ScreenSize && ScreenSize != 0) {
                    //
                    // If ScreenSize == 0 we have the console redirected so don't
                    //  block updates
                    //
                    ScreenCount = 0;
                  }
                  //
                  // If this is not a multi-function device, we can leave the loop
                  // to deal with the next device.
                  //
                  if (Func == 0 && ((PciHeader.HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00)) {
                    break;
                  }
                }
              }
            }
          }
          //
          // If Descriptor is NULL, Configuration() returns EFI_UNSUPPRORED,
          // we assume the bus range is 0~PCI_MAX_BUS. After enumerated all
          // devices on all bus, we can leave loop.
          //
          if (Descriptors == NULL) {
            break;
          }
        }
      }

      Status = EFI_SUCCESS;
      goto Done;
    }

    ExplainData                   = FALSE;
    Segment                       = 0;
    Bus                           = 0;
    Device                        = 0;
    Func                          = 0;
    ExtendedCapability          = 0xFFFF;
    if (ShellCommandLineGetFlag(Package, L"-i")) {
      ExplainData = TRUE;
    }

    Temp = ShellCommandLineGetValue(Package, L"-s");
    if (Temp != NULL) {
      //
      // Input converted to hexadecimal number.
      //
      if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
        Segment = (UINT16) RetVal;
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // The first Argument(except "-i") is assumed to be Bus number, second
    // to be Device number, and third to be Func number.
    //
    Temp = ShellCommandLineGetRawValue(Package, 1);
    if (Temp != NULL) {
      //
      // Input converted to hexadecimal number.
      //
      if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
        Bus = (UINT16) RetVal;
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (Bus > PCI_MAX_BUS) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }
    Temp = ShellCommandLineGetRawValue(Package, 2);
    if (Temp != NULL) {
      //
      // Input converted to hexadecimal number.
      //
      if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
        Device = (UINT16) RetVal;
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (Device > PCI_MAX_DEVICE){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    Temp = ShellCommandLineGetRawValue(Package, 3);
    if (Temp != NULL) {
      //
      // Input converted to hexadecimal number.
      //
      if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
        Func = (UINT16) RetVal;
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (Func > PCI_MAX_FUNC){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    Temp = ShellCommandLineGetValue (Package, L"-ec");
    if (Temp != NULL) {
      //
      // Input converted to hexadecimal number.
      //
      if (!EFI_ERROR (ShellConvertStringToUint64 (Temp, &RetVal, TRUE, TRUE))) {
        ExtendedCapability = (UINT16) RetVal;
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV_HEX), gShellDebug1HiiHandle, L"pci", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Find the protocol interface who's in charge of current segment, and its
    // bus range covers the current bus
    //
    Status = PciFindProtocolInterface (
              HandleBuf,
              HandleCount,
              Segment,
              Bus,
              &IoDev
             );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx(
        -1, -1, NULL, STRING_TOKEN (STR_PCI_NO_FIND), gShellDebug1HiiHandle, L"pci",
        Segment,
        Bus
       );
      ShellStatus = SHELL_NOT_FOUND;
      goto Done;
    }

    Address = EFI_PCI_ADDRESS (Bus, Device, Func, 0);
    Status = IoDev->Pci.Read (
                          IoDev,
                          EfiPciWidthUint8,
                          Address,
                          sizeof (ConfigSpace),
                          &ConfigSpace
                         );

    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_NO_CFG), gShellDebug1HiiHandle, L"pci");
      ShellStatus = SHELL_ACCESS_DENIED;
      goto Done;
    }

    mConfigSpace = &ConfigSpace;
    ShellPrintHiiEx(
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_PCI_INFO),
      gShellDebug1HiiHandle,
      Segment,
      Bus,
      Device,
      Func,
      Segment,
      Bus,
      Device,
      Func
     );

    //
    // Dump standard header of configuration space
    //
    SizeOfHeader = sizeof (ConfigSpace.Common) + sizeof (ConfigSpace.NonCommon);

    DumpHex (2, 0, SizeOfHeader, &ConfigSpace);
    ShellPrintEx(-1,-1, L"\r\n");

    //
    // Dump device dependent Part of configuration space
    //
    DumpHex (
      2,
      SizeOfHeader,
      sizeof (ConfigSpace) - SizeOfHeader,
      ConfigSpace.Data
     );

    ExtendedConfigSpace = NULL;
    ExtendedConfigSize  = 0;
    PcieCapabilityPtr = LocatePciCapability (&ConfigSpace, EFI_PCI_CAPABILITY_ID_PCIEXP);
    if (PcieCapabilityPtr != 0) {
      ExtendedConfigSize  = 0x1000 - EFI_PCIE_CAPABILITY_BASE_OFFSET;
      ExtendedConfigSpace = AllocatePool (ExtendedConfigSize);
      if (ExtendedConfigSpace != NULL) {
        Status = IoDev->Pci.Read (
                              IoDev,
                              EfiPciWidthUint32,
                              EFI_PCI_ADDRESS (Bus, Device, Func, EFI_PCIE_CAPABILITY_BASE_OFFSET),
                              ExtendedConfigSize / sizeof (UINT32),
                              ExtendedConfigSpace
                              );
        if (EFI_ERROR (Status)) {
          SHELL_FREE_NON_NULL (ExtendedConfigSpace);
        }
      }
    }

    if ((ExtendedConfigSpace != NULL) && !ShellGetExecutionBreakFlag ()) {
      //
      // Print the PciEx extend space in raw bytes ( 0xFF-0xFFF)
      //
      ShellPrintEx (-1, -1, L"\r\n%HStart dumping PCIex extended configuration space (0x100 - 0xFFF).%N\r\n\r\n");

      DumpHex (
        2,
        EFI_PCIE_CAPABILITY_BASE_OFFSET,
        ExtendedConfigSize,
        ExtendedConfigSpace
        );
    }

    //
    // If "-i" appears in command line, interpret data in configuration space
    //
    if (ExplainData) {
      PciExplainPci (&ConfigSpace, Address, IoDev);
      if ((ExtendedConfigSpace != NULL) && !ShellGetExecutionBreakFlag ()) {
        PciExplainPciExpress (
          (PCI_CAPABILITY_PCIEXP *) ((UINT8 *) &ConfigSpace + PcieCapabilityPtr),
          ExtendedConfigSpace,
          ExtendedConfigSize,
          ExtendedCapability
          );
      }
    }
  }
Done:
  if (HandleBuf != NULL) {
    FreePool (HandleBuf);
  }
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  mConfigSpace = NULL;
  return ShellStatus;
}

/**
  This function finds out the protocol which is in charge of the given
  segment, and its bus range covers the current bus number. It lookes
  each instances of RootBridgeIoProtocol handle, until the one meets the
  criteria is found.

  @param[in] HandleBuf       Buffer which holds all PCI_ROOT_BRIDIGE_IO_PROTOCOL handles.
  @param[in] HandleCount     Count of all PCI_ROOT_BRIDIGE_IO_PROTOCOL handles.
  @param[in] Segment         Segment number of device we are dealing with.
  @param[in] Bus             Bus number of device we are dealing with.
  @param[out] IoDev          Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS             The command completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.

**/
EFI_STATUS
PciFindProtocolInterface (
  IN  EFI_HANDLE                            *HandleBuf,
  IN  UINTN                                 HandleCount,
  IN  UINT16                                Segment,
  IN  UINT16                                Bus,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       **IoDev
  )
{
  UINTN                             Index;
  EFI_STATUS                        Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptors;
  UINT16                            MinBus;
  UINT16                            MaxBus;
  BOOLEAN                           IsEnd;

  //
  // Go through all handles, until the one meets the criteria is found
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = PciGetProtocolAndResource (HandleBuf[Index], IoDev, &Descriptors);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // When Descriptors == NULL, the Configuration() is not implemented,
    // so we only check the Segment number
    //
    if (Descriptors == NULL && Segment == (*IoDev)->SegmentNumber) {
      return EFI_SUCCESS;
    }

    if ((*IoDev)->SegmentNumber != Segment) {
      continue;
    }

    while (TRUE) {
      Status = PciGetNextBusRange (&Descriptors, &MinBus, &MaxBus, &IsEnd);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (IsEnd) {
        break;
      }

      if (MinBus <= Bus && MaxBus >= Bus) {
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function gets the protocol interface from the given handle, and
  obtains its address space descriptors.

  @param[in] Handle          The PCI_ROOT_BRIDIGE_IO_PROTOCOL handle.
  @param[out] IoDev          Handle used to access configuration space of PCI device.
  @param[out] Descriptors    Points to the address space descriptors.

  @retval EFI_SUCCESS     The command completed successfully
**/
EFI_STATUS
PciGetProtocolAndResource (
  IN  EFI_HANDLE                            Handle,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       **IoDev,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     **Descriptors
  )
{
  EFI_STATUS  Status;

  //
  // Get inferface from protocol
  //
  Status = gBS->HandleProtocol (
                Handle,
                &gEfiPciRootBridgeIoProtocolGuid,
                (VOID**)IoDev
               );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Call Configuration() to get address space descriptors
  //
  Status = (*IoDev)->Configuration (*IoDev, (VOID**)Descriptors);
  if (Status == EFI_UNSUPPORTED) {
    *Descriptors = NULL;
    return EFI_SUCCESS;

  } else {
    return Status;
  }
}

/**
  This function get the next bus range of given address space descriptors.
  It also moves the pointer backward a node, to get prepared to be called
  again.

  @param[in, out] Descriptors Points to current position of a serial of address space
                              descriptors.
  @param[out] MinBus          The lower range of bus number.
  @param[out] MaxBus          The upper range of bus number.
  @param[out] IsEnd           Meet end of the serial of descriptors.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciGetNextBusRange (
  IN OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors,
  OUT    UINT16                             *MinBus,
  OUT    UINT16                             *MaxBus,
  OUT    BOOLEAN                            *IsEnd
  )
{
  *IsEnd = FALSE;

  //
  // When *Descriptors is NULL, Configuration() is not implemented, so assume
  // range is 0~PCI_MAX_BUS
  //
  if ((*Descriptors) == NULL) {
    *MinBus = 0;
    *MaxBus = PCI_MAX_BUS;
    return EFI_SUCCESS;
  }
  //
  // *Descriptors points to one or more address space descriptors, which
  // ends with a end tagged descriptor. Examine each of the descriptors,
  // if a bus typed one is found and its bus range covers bus, this handle
  // is the handle we are looking for.
  //

  while ((*Descriptors)->Desc != ACPI_END_TAG_DESCRIPTOR) {
    if ((*Descriptors)->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
      *MinBus = (UINT16) (*Descriptors)->AddrRangeMin;
      *MaxBus = (UINT16) (*Descriptors)->AddrRangeMax;
      (*Descriptors)++;
      return (EFI_SUCCESS);
    }

    (*Descriptors)++;
  }

  if ((*Descriptors)->Desc == ACPI_END_TAG_DESCRIPTOR) {
    *IsEnd = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Explain the data in PCI configuration space. The part which is common for
  PCI device and bridge is interpreted in this function. It calls other
  functions to interpret data unique for device or bridge.

  @param[in] ConfigSpace     Data in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.
**/
VOID
PciExplainPci (
  IN PCI_CONFIG_SPACE                       *ConfigSpace,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  )
{
  PCI_DEVICE_INDEPENDENT_REGION *Common;
  PCI_HEADER_TYPE               HeaderType;

  Common = &(ConfigSpace->Common);

  ShellPrintEx (-1, -1, L"\r\n");

  //
  // Print Vendor Id and Device Id
  //
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_VID_DID), gShellDebug1HiiHandle,
    INDEX_OF (&(Common->VendorId)),
    Common->VendorId,
    INDEX_OF (&(Common->DeviceId)),
    Common->DeviceId
   );

  //
  // Print register Command
  //
  PciExplainCommand (&(Common->Command));

  //
  // Print register Status
  //
  PciExplainStatus (&(Common->Status), TRUE, PciUndefined);

  //
  // Print register Revision ID
  //
  ShellPrintEx(-1, -1, L"\r\n");
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_RID), gShellDebug1HiiHandle,
    INDEX_OF (&(Common->RevisionID)),
    Common->RevisionID
   );

  //
  // Print register BIST
  //
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_BIST), gShellDebug1HiiHandle, INDEX_OF (&(Common->BIST)));
  if ((Common->BIST & BIT7) != 0) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_CAP), gShellDebug1HiiHandle, 0x0f & Common->BIST);
  } else {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_PCI_LINE_CAP_NO), gShellDebug1HiiHandle);
  }
  //
  // Print register Cache Line Size
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CACHE_LINE_SIZE),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Common->CacheLineSize)),
    Common->CacheLineSize
   );

  //
  // Print register Latency Timer
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_LATENCY_TIMER),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Common->LatencyTimer)),
    Common->LatencyTimer
   );

  //
  // Print register Header Type
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_HEADER_TYPE),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Common->HeaderType)),
    Common->HeaderType
   );

  if ((Common->HeaderType & BIT7) != 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MULTI_FUNCTION), gShellDebug1HiiHandle);

  } else {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_SINGLE_FUNCTION), gShellDebug1HiiHandle);
  }

  HeaderType = (PCI_HEADER_TYPE)(UINT8) (Common->HeaderType & 0x7f);
  switch (HeaderType) {
  case PciDevice:
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_PCI_DEVICE), gShellDebug1HiiHandle);
    break;

  case PciP2pBridge:
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_P2P_BRIDGE), gShellDebug1HiiHandle);
    break;

  case PciCardBusBridge:
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_CARDBUS_BRIDGE), gShellDebug1HiiHandle);
    break;

  default:
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RESERVED), gShellDebug1HiiHandle);
    HeaderType = PciUndefined;
  }

  //
  // Print register Class Code
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_CLASS), gShellDebug1HiiHandle);
  PciPrintClassCode ((UINT8 *) Common->ClassCode, TRUE);
  ShellPrintEx (-1, -1, L"\r\n");
}

/**
  Explain the device specific part of data in PCI configuration space.

  @param[in] Device          Data in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainDeviceData (
  IN PCI_DEVICE_HEADER_TYPE_REGION          *Device,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  )
{
  UINTN       Index;
  BOOLEAN     BarExist;
  EFI_STATUS  Status;
  UINTN       BarCount;

  //
  // Print Base Address Registers(Bar). When Bar = 0, this Bar does not
  // exist. If these no Bar for this function, print "none", otherwise
  // list detail information about this Bar.
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BASE_ADDR), gShellDebug1HiiHandle, INDEX_OF (Device->Bar));

  BarExist  = FALSE;
  BarCount  = sizeof (Device->Bar) / sizeof (Device->Bar[0]);
  for (Index = 0; Index < BarCount; Index++) {
    if (Device->Bar[Index] == 0) {
      continue;
    }

    if (!BarExist) {
      BarExist = TRUE;
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_START_TYPE), gShellDebug1HiiHandle);
      ShellPrintEx (-1, -1, L"  --------------------------------------------------------------------------");
    }

    Status = PciExplainBar (
              &(Device->Bar[Index]),
              &(mConfigSpace->Common.Command),
              Address,
              IoDev,
              &Index
             );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (!BarExist) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NONE), gShellDebug1HiiHandle);

  } else {
    ShellPrintEx (-1, -1, L"\r\n  --------------------------------------------------------------------------");
  }

  //
  // Print register Expansion ROM Base Address
  //
  if ((Device->ExpansionRomBar & BIT0) == 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_EXPANSION_ROM_DISABLED), gShellDebug1HiiHandle, INDEX_OF (&(Device->ExpansionRomBar)));

  } else {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_EXPANSION_ROM_BASE),
      gShellDebug1HiiHandle,
      INDEX_OF (&(Device->ExpansionRomBar)),
      Device->ExpansionRomBar
     );
  }
  //
  // Print register Cardbus CIS ptr
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CARDBUS_CIS),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->CISPtr)),
    Device->CISPtr
   );

  //
  // Print register Sub-vendor ID and subsystem ID
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SUB_VENDOR_ID),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->SubsystemVendorID)),
    Device->SubsystemVendorID
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SUBSYSTEM_ID),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->SubsystemID)),
    Device->SubsystemID
   );

  //
  // Print register Capabilities Ptr
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CAPABILITIES_PTR),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->CapabilityPtr)),
    Device->CapabilityPtr
   );

  //
  // Print register Interrupt Line and interrupt pin
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_INTERRUPT_LINE),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->InterruptLine)),
    Device->InterruptLine
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_INTERRUPT_PIN),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->InterruptPin)),
    Device->InterruptPin
   );

  //
  // Print register Min_Gnt and Max_Lat
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MIN_GNT),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->MinGnt)),
    Device->MinGnt
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MAX_LAT),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Device->MaxLat)),
    Device->MaxLat
   );

  return EFI_SUCCESS;
}

/**
  Explain the bridge specific part of data in PCI configuration space.

  @param[in] Bridge          Bridge specific data region in PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBridgeData (
  IN  PCI_BRIDGE_CONTROL_REGISTER           *Bridge,
  IN  UINT64                                Address,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       *IoDev
  )
{
  UINTN       Index;
  BOOLEAN     BarExist;
  UINTN       BarCount;
  UINT32      IoAddress32;
  EFI_STATUS  Status;

  //
  // Print Base Address Registers. When Bar = 0, this Bar does not
  // exist. If these no Bar for this function, print "none", otherwise
  // list detail information about this Bar.
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BASE_ADDRESS), gShellDebug1HiiHandle, INDEX_OF (&(Bridge->Bar)));

  BarExist  = FALSE;
  BarCount  = sizeof (Bridge->Bar) / sizeof (Bridge->Bar[0]);

  for (Index = 0; Index < BarCount; Index++) {
    if (Bridge->Bar[Index] == 0) {
      continue;
    }

    if (!BarExist) {
      BarExist = TRUE;
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_START_TYPE_2), gShellDebug1HiiHandle);
      ShellPrintEx (-1, -1, L"  --------------------------------------------------------------------------");
    }

    Status = PciExplainBar (
              &(Bridge->Bar[Index]),
              &(mConfigSpace->Common.Command),
              Address,
              IoDev,
              &Index
             );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (!BarExist) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NONE), gShellDebug1HiiHandle);
  } else {
    ShellPrintEx (-1, -1, L"\r\n  --------------------------------------------------------------------------");
  }

  //
  // Expansion register ROM Base Address
  //
  if ((Bridge->ExpansionRomBAR & BIT0) == 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NO_EXPANSION_ROM), gShellDebug1HiiHandle, INDEX_OF (&(Bridge->ExpansionRomBAR)));

  } else {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_EXPANSION_ROM_BASE_2),
      gShellDebug1HiiHandle,
      INDEX_OF (&(Bridge->ExpansionRomBAR)),
      Bridge->ExpansionRomBAR
     );
  }
  //
  // Print Bus Numbers(Primary, Secondary, and Subordinate
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_BUS_NUMBERS),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->PrimaryBus)),
    INDEX_OF (&(Bridge->SecondaryBus)),
    INDEX_OF (&(Bridge->SubordinateBus))
   );

  ShellPrintEx (-1, -1, L"               ------------------------------------------------------\r\n");

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BRIDGE), gShellDebug1HiiHandle, Bridge->PrimaryBus);
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BRIDGE), gShellDebug1HiiHandle, Bridge->SecondaryBus);
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BRIDGE), gShellDebug1HiiHandle, Bridge->SubordinateBus);

  //
  // Print register Secondary Latency Timer
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SECONDARY_TIMER),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->SecondaryLatencyTimer)),
    Bridge->SecondaryLatencyTimer
   );

  //
  // Print register Secondary Status
  //
  PciExplainStatus (&(Bridge->SecondaryStatus), FALSE, PciP2pBridge);

  //
  // Print I/O and memory ranges this bridge forwards. There are 3 resource
  // types: I/O, memory, and pre-fetchable memory. For each resource type,
  // base and limit address are listed.
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RESOURCE_TYPE), gShellDebug1HiiHandle);
  ShellPrintEx (-1, -1, L"----------------------------------------------------------------------\r\n");

  //
  // IO Base & Limit
  //
  IoAddress32 = (Bridge->IoBaseUpper16 << 16 | Bridge->IoBase << 8);
  IoAddress32 &= 0xfffff000;
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_TWO_VARS),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->IoBase)),
    IoAddress32
   );

  IoAddress32 = (Bridge->IoLimitUpper16 << 16 | Bridge->IoLimit << 8);
  IoAddress32 |= 0x00000fff;
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_ONE_VAR), gShellDebug1HiiHandle, IoAddress32);

  //
  // Memory Base & Limit
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MEMORY),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->MemoryBase)),
    (Bridge->MemoryBase << 16) & 0xfff00000
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_ONE_VAR),
    gShellDebug1HiiHandle,
    (Bridge->MemoryLimit << 16) | 0x000fffff
   );

  //
  // Pre-fetch-able Memory Base & Limit
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_PREFETCHABLE),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->PrefetchableMemoryBase)),
    Bridge->PrefetchableBaseUpper32,
    (Bridge->PrefetchableMemoryBase << 16) & 0xfff00000
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_TWO_VARS_2),
    gShellDebug1HiiHandle,
    Bridge->PrefetchableLimitUpper32,
    (Bridge->PrefetchableMemoryLimit << 16) | 0x000fffff
   );

  //
  // Print register Capabilities Pointer
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CAPABILITIES_PTR_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->CapabilityPtr)),
    Bridge->CapabilityPtr
   );

  //
  // Print register Bridge Control
  //
  PciExplainBridgeControl (&(Bridge->BridgeControl), PciP2pBridge);

  //
  // Print register Interrupt Line & PIN
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_INTERRUPT_LINE_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->InterruptLine)),
    Bridge->InterruptLine
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_INTERRUPT_PIN),
    gShellDebug1HiiHandle,
    INDEX_OF (&(Bridge->InterruptPin)),
    Bridge->InterruptPin
   );

  return EFI_SUCCESS;
}

/**
  Explain the Base Address Register(Bar) in PCI configuration space.

  @param[in] Bar              Points to the Base Address Register intended to interpret.
  @param[in] Command          Points to the register Command.
  @param[in] Address          Address used to access configuration space of this PCI device.
  @param[in] IoDev            Handle used to access configuration space of PCI device.
  @param[in, out] Index       The Index.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBar (
  IN UINT32                                 *Bar,
  IN UINT16                                 *Command,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev,
  IN OUT UINTN                              *Index
  )
{
  UINT16  OldCommand;
  UINT16  NewCommand;
  UINT64  Bar64;
  UINT32  OldBar32;
  UINT32  NewBar32;
  UINT64  OldBar64;
  UINT64  NewBar64;
  BOOLEAN IsMem;
  BOOLEAN IsBar32;
  UINT64  RegAddress;

  IsBar32   = TRUE;
  Bar64     = 0;
  NewBar32  = 0;
  NewBar64  = 0;

  //
  // According the bar type, list detail about this bar, for example: 32 or
  // 64 bits; pre-fetchable or not.
  //
  if ((*Bar & BIT0) == 0) {
    //
    // This bar is of memory type
    //
    IsMem = TRUE;

    if ((*Bar & BIT1) == 0 && (*Bar & BIT2) == 0) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BAR), gShellDebug1HiiHandle, *Bar & 0xfffffff0);
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MEM), gShellDebug1HiiHandle);
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_32_BITS), gShellDebug1HiiHandle);

    } else if ((*Bar & BIT1) == 0 && (*Bar & BIT2) != 0) {
      Bar64 = 0x0;
      CopyMem (&Bar64, Bar, sizeof (UINT64));
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_ONE_VAR_2), gShellDebug1HiiHandle, (UINT32) RShiftU64 ((Bar64 & 0xfffffffffffffff0ULL), 32));
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_ONE_VAR_3), gShellDebug1HiiHandle, (UINT32) (Bar64 & 0xfffffffffffffff0ULL));
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MEM), gShellDebug1HiiHandle);
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_64_BITS), gShellDebug1HiiHandle);
      IsBar32 = FALSE;
      *Index += 1;

    } else {
      //
      // Reserved
      //
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_BAR), gShellDebug1HiiHandle, *Bar & 0xfffffff0);
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MEM_2), gShellDebug1HiiHandle);
    }

    if ((*Bar & BIT3) == 0) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NO), gShellDebug1HiiHandle);

    } else {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_YES), gShellDebug1HiiHandle);
    }

  } else {
    //
    // This bar is of io type
    //
    IsMem = FALSE;
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_ONE_VAR_4), gShellDebug1HiiHandle, *Bar & 0xfffffffc);
    ShellPrintEx (-1, -1, L"I/O                               ");
  }

  //
  // Get BAR length(or the amount of resource this bar demands for). To get
  // Bar length, first we should temporarily disable I/O and memory access
  // of this function(by set bits in the register Command), then write all
  // "1"s to this bar. The bar value read back is the amount of resource
  // this bar demands for.
  //
  //
  // Disable io & mem access
  //
  OldCommand  = *Command;
  NewCommand  = (UINT16) (OldCommand & 0xfffc);
  RegAddress  = Address | INDEX_OF (Command);
  IoDev->Pci.Write (IoDev, EfiPciWidthUint16, RegAddress, 1, &NewCommand);

  RegAddress = Address | INDEX_OF (Bar);

  //
  // Read after write the BAR to get the size
  //
  if (IsBar32) {
    OldBar32  = *Bar;
    NewBar32  = 0xffffffff;

    IoDev->Pci.Write (IoDev, EfiPciWidthUint32, RegAddress, 1, &NewBar32);
    IoDev->Pci.Read (IoDev, EfiPciWidthUint32, RegAddress, 1, &NewBar32);
    IoDev->Pci.Write (IoDev, EfiPciWidthUint32, RegAddress, 1, &OldBar32);

    if (IsMem) {
      NewBar32  = NewBar32 & 0xfffffff0;
      NewBar32  = (~NewBar32) + 1;

    } else {
      NewBar32  = NewBar32 & 0xfffffffc;
      NewBar32  = (~NewBar32) + 1;
      NewBar32  = NewBar32 & 0x0000ffff;
    }
  } else {

    OldBar64 = 0x0;
    CopyMem (&OldBar64, Bar, sizeof (UINT64));
    NewBar64 = 0xffffffffffffffffULL;

    IoDev->Pci.Write (IoDev, EfiPciWidthUint32, RegAddress, 2, &NewBar64);
    IoDev->Pci.Read (IoDev, EfiPciWidthUint32, RegAddress, 2, &NewBar64);
    IoDev->Pci.Write (IoDev, EfiPciWidthUint32, RegAddress, 2, &OldBar64);

    if (IsMem) {
      NewBar64  = NewBar64 & 0xfffffffffffffff0ULL;
      NewBar64  = (~NewBar64) + 1;

    } else {
      NewBar64  = NewBar64 & 0xfffffffffffffffcULL;
      NewBar64  = (~NewBar64) + 1;
      NewBar64  = NewBar64 & 0x000000000000ffff;
    }
  }
  //
  // Enable io & mem access
  //
  RegAddress = Address | INDEX_OF (Command);
  IoDev->Pci.Write (IoDev, EfiPciWidthUint16, RegAddress, 1, &OldCommand);

  if (IsMem) {
    if (IsBar32) {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NEWBAR_32), gShellDebug1HiiHandle, NewBar32);
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NEWBAR_32_2), gShellDebug1HiiHandle, NewBar32 + (*Bar & 0xfffffff0) - 1);

    } else {
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RSHIFT), gShellDebug1HiiHandle, (UINT32) RShiftU64 (NewBar64, 32));
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RSHIFT), gShellDebug1HiiHandle, (UINT32) NewBar64);
      ShellPrintEx (-1, -1, L"  ");
      ShellPrintHiiEx(-1, -1, NULL,
        STRING_TOKEN (STR_PCI2_RSHIFT),
        gShellDebug1HiiHandle,
        (UINT32) RShiftU64 ((NewBar64 + (Bar64 & 0xfffffffffffffff0ULL) - 1), 32)
       );
      ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RSHIFT), gShellDebug1HiiHandle, (UINT32) (NewBar64 + (Bar64 & 0xfffffffffffffff0ULL) - 1));

    }
  } else {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NEWBAR_32_3), gShellDebug1HiiHandle, NewBar32);
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NEWBAR_32_4), gShellDebug1HiiHandle, NewBar32 + (*Bar & 0xfffffffc) - 1);
  }

  return EFI_SUCCESS;
}

/**
  Explain the cardbus specific part of data in PCI configuration space.

  @param[in] CardBus         CardBus specific region of PCI configuration space.
  @param[in] Address         Address used to access configuration space of this PCI device.
  @param[in] IoDev           Handle used to access configuration space of PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainCardBusData (
  IN PCI_CARDBUS_CONTROL_REGISTER           *CardBus,
  IN UINT64                                 Address,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *IoDev
  )
{
  BOOLEAN           Io32Bit;
  PCI_CARDBUS_DATA  *CardBusData;

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CARDBUS_SOCKET),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->CardBusSocketReg)),
    CardBus->CardBusSocketReg
   );

  //
  // Print Secondary Status
  //
  PciExplainStatus (&(CardBus->SecondaryStatus), FALSE, PciCardBusBridge);

  //
  // Print Bus Numbers(Primary bus number, CardBus bus number, and
  // Subordinate bus number
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_BUS_NUMBERS_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->PciBusNumber)),
    INDEX_OF (&(CardBus->CardBusBusNumber)),
    INDEX_OF (&(CardBus->SubordinateBusNumber))
   );

  ShellPrintEx (-1, -1, L"               ------------------------------------------------------\r\n");

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_CARDBUS), gShellDebug1HiiHandle, CardBus->PciBusNumber);
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_CARDBUS_2), gShellDebug1HiiHandle, CardBus->CardBusBusNumber);
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_CARDBUS_3), gShellDebug1HiiHandle, CardBus->SubordinateBusNumber);

  //
  // Print CardBus Latency Timer
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_CARDBUS_LATENCY),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->CardBusLatencyTimer)),
    CardBus->CardBusLatencyTimer
   );

  //
  // Print Memory/Io ranges this cardbus bridge forwards
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RESOURCE_TYPE_2), gShellDebug1HiiHandle);
  ShellPrintEx (-1, -1, L"----------------------------------------------------------------------\r\n");

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MEM_3),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->MemoryBase0)),
    CardBus->BridgeControl & BIT8 ? L"    Prefetchable" : L"Non-Prefetchable",
    CardBus->MemoryBase0 & 0xfffff000,
    CardBus->MemoryLimit0 | 0x00000fff
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MEM_3),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->MemoryBase1)),
    CardBus->BridgeControl & BIT9 ? L"    Prefetchable" : L"Non-Prefetchable",
    CardBus->MemoryBase1 & 0xfffff000,
    CardBus->MemoryLimit1 | 0x00000fff
   );

  Io32Bit = (BOOLEAN) (CardBus->IoBase0 & BIT0);
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_IO_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->IoBase0)),
    Io32Bit ? L"          32 bit" : L"          16 bit",
    CardBus->IoBase0 & (Io32Bit ? 0xfffffffc : 0x0000fffc),
    (CardBus->IoLimit0 & (Io32Bit ? 0xffffffff : 0x0000ffff)) | 0x00000003
   );

  Io32Bit = (BOOLEAN) (CardBus->IoBase1 & BIT0);
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_IO_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->IoBase1)),
    Io32Bit ? L"          32 bit" : L"          16 bit",
    CardBus->IoBase1 & (Io32Bit ? 0xfffffffc : 0x0000fffc),
    (CardBus->IoLimit1 & (Io32Bit ? 0xffffffff : 0x0000ffff)) | 0x00000003
   );

  //
  // Print register Interrupt Line & PIN
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_INTERRUPT_LINE_3),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBus->InterruptLine)),
    CardBus->InterruptLine,
    INDEX_OF (&(CardBus->InterruptPin)),
    CardBus->InterruptPin
   );

  //
  // Print register Bridge Control
  //
  PciExplainBridgeControl (&(CardBus->BridgeControl), PciCardBusBridge);

  //
  // Print some registers in data region of PCI configuration space for cardbus
  // bridge. Fields include: Sub VendorId, Subsystem ID, and Legacy Mode Base
  // Address.
  //
  CardBusData = (PCI_CARDBUS_DATA *) ((UINT8 *) CardBus + sizeof (PCI_CARDBUS_CONTROL_REGISTER));

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SUB_VENDOR_ID_2),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBusData->SubVendorId)),
    CardBusData->SubVendorId,
    INDEX_OF (&(CardBusData->SubSystemId)),
    CardBusData->SubSystemId
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_OPTIONAL),
    gShellDebug1HiiHandle,
    INDEX_OF (&(CardBusData->LegacyBase)),
    CardBusData->LegacyBase
   );

  return EFI_SUCCESS;
}

/**
  Explain each meaningful bit of register Status. The definition of Status is
  slightly different depending on the PCI header type.

  @param[in] Status          Points to the content of register Status.
  @param[in] MainStatus      Indicates if this register is main status(not secondary
                             status).
  @param[in] HeaderType      Header type of this PCI device.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainStatus (
  IN UINT16                                 *Status,
  IN BOOLEAN                                MainStatus,
  IN PCI_HEADER_TYPE                        HeaderType
  )
{
  if (MainStatus) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_STATUS), gShellDebug1HiiHandle, INDEX_OF (Status), *Status);

  } else {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_SECONDARY_STATUS), gShellDebug1HiiHandle, INDEX_OF (Status), *Status);
  }

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_NEW_CAPABILITIES), gShellDebug1HiiHandle, (*Status & BIT4) != 0);

  //
  // Bit 5 is meaningless for CardBus Bridge
  //
  if (HeaderType == PciCardBusBridge) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_66_CAPABLE), gShellDebug1HiiHandle, (*Status & BIT5) != 0);

  } else {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_66_CAPABLE_2), gShellDebug1HiiHandle, (*Status & BIT5) != 0);
  }

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_FAST_BACK), gShellDebug1HiiHandle, (*Status & BIT7) != 0);

  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MASTER_DATA), gShellDebug1HiiHandle, (*Status & BIT8) != 0);
  //
  // Bit 9 and bit 10 together decides the DEVSEL timing
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_DEVSEL_TIMING), gShellDebug1HiiHandle);
  if ((*Status & BIT9) == 0 && (*Status & BIT10) == 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_FAST), gShellDebug1HiiHandle);

  } else if ((*Status & BIT9) != 0 && (*Status & BIT10) == 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_MEDIUM), gShellDebug1HiiHandle);

  } else if ((*Status & BIT9) == 0 && (*Status & BIT10) != 0) {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_SLOW), gShellDebug1HiiHandle);

  } else {
    ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_RESERVED_2), gShellDebug1HiiHandle);
  }

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SIGNALED_TARGET),
    gShellDebug1HiiHandle,
    (*Status & BIT11) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_RECEIVED_TARGET),
    gShellDebug1HiiHandle,
    (*Status & BIT12) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_RECEIVED_MASTER),
    gShellDebug1HiiHandle,
    (*Status & BIT13) != 0
   );

  if (MainStatus) {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_SIGNALED_ERROR),
      gShellDebug1HiiHandle,
      (*Status & BIT14) != 0
     );

  } else {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_RECEIVED_ERROR),
      gShellDebug1HiiHandle,
      (*Status & BIT14) != 0
     );
  }

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_DETECTED_ERROR),
    gShellDebug1HiiHandle,
    (*Status & BIT15) != 0
   );

  return EFI_SUCCESS;
}

/**
  Explain each meaningful bit of register Command.

  @param[in] Command         Points to the content of register Command.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainCommand (
  IN UINT16                                 *Command
  )
{
  //
  // Print the binary value of register Command
  //
  ShellPrintHiiEx(-1, -1, NULL,STRING_TOKEN (STR_PCI2_COMMAND), gShellDebug1HiiHandle, INDEX_OF (Command), *Command);

  //
  // Explain register Command bit by bit
  //
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SPACE_ACCESS_DENIED),
    gShellDebug1HiiHandle,
    (*Command & BIT0) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MEMORY_SPACE),
    gShellDebug1HiiHandle,
    (*Command & BIT1) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_BEHAVE_BUS_MASTER),
    gShellDebug1HiiHandle,
    (*Command & BIT2) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MONITOR_SPECIAL_CYCLE),
    gShellDebug1HiiHandle,
    (*Command & BIT3) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MEM_WRITE_INVALIDATE),
    gShellDebug1HiiHandle,
    (*Command & BIT4) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_PALETTE_SNOOPING),
    gShellDebug1HiiHandle,
    (*Command & BIT5) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_ASSERT_PERR),
    gShellDebug1HiiHandle,
    (*Command & BIT6) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_DO_ADDR_STEPPING),
    gShellDebug1HiiHandle,
    (*Command & BIT7) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SERR_DRIVER),
    gShellDebug1HiiHandle,
    (*Command & BIT8) != 0
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_FAST_BACK_2),
    gShellDebug1HiiHandle,
    (*Command & BIT9) != 0
   );

  return EFI_SUCCESS;
}

/**
  Explain each meaningful bit of register Bridge Control.

  @param[in] BridgeControl   Points to the content of register Bridge Control.
  @param[in] HeaderType      The headertype.

  @retval EFI_SUCCESS     The command completed successfully.
**/
EFI_STATUS
PciExplainBridgeControl (
  IN UINT16                                 *BridgeControl,
  IN PCI_HEADER_TYPE                        HeaderType
  )
{
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_BRIDGE_CONTROL),
    gShellDebug1HiiHandle,
    INDEX_OF (BridgeControl),
    *BridgeControl
   );

  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_PARITY_ERROR),
    gShellDebug1HiiHandle,
    (*BridgeControl & BIT0) != 0
   );
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_SERR_ENABLE),
    gShellDebug1HiiHandle,
    (*BridgeControl & BIT1) != 0
   );
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_ISA_ENABLE),
    gShellDebug1HiiHandle,
    (*BridgeControl & BIT2) != 0
   );
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_VGA_ENABLE),
    gShellDebug1HiiHandle,
    (*BridgeControl & BIT3) != 0
   );
  ShellPrintHiiEx(-1, -1, NULL,
    STRING_TOKEN (STR_PCI2_MASTER_ABORT),
    gShellDebug1HiiHandle,
    (*BridgeControl & BIT5) != 0
   );

  //
  // Register Bridge Control has some slight differences between P2P bridge
  // and Cardbus bridge from bit 6 to bit 11.
  //
  if (HeaderType == PciP2pBridge) {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_SECONDARY_BUS_RESET),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT6) != 0
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_FAST_ENABLE),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT7) != 0
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_PRIMARY_DISCARD_TIMER),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT8)!=0 ? L"2^10" : L"2^15"
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_SECONDARY_DISCARD_TIMER),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT9)!=0 ? L"2^10" : L"2^15"
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_DISCARD_TIMER_STATUS),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT10) != 0
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_DISCARD_TIMER_SERR),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT11) != 0
     );

  } else {
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_CARDBUS_RESET),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT6) != 0
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_IREQ_ENABLE),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT7) != 0
     );
    ShellPrintHiiEx(-1, -1, NULL,
      STRING_TOKEN (STR_PCI2_WRITE_POSTING_ENABLE),
      gShellDebug1HiiHandle,
      (*BridgeControl & BIT10) != 0
     );
  }

  return EFI_SUCCESS;
}

/**
  Locate capability register block per capability ID.

  @param[in] ConfigSpace       Data in PCI configuration space.
  @param[in] CapabilityId      The capability ID.

  @return   The offset of the register block per capability ID,
            or 0 if the register block cannot be found.
**/
UINT8
LocatePciCapability (
  IN PCI_CONFIG_SPACE   *ConfigSpace,
  IN UINT8              CapabilityId
  )
{
  UINT8                   CapabilityPtr;
  EFI_PCI_CAPABILITY_HDR  *CapabilityEntry;

  //
  // To check the cpability of this device supports
  //
  if ((ConfigSpace->Common.Status & EFI_PCI_STATUS_CAPABILITY) == 0) {
    return 0;
  }

  switch ((PCI_HEADER_TYPE)(ConfigSpace->Common.HeaderType & 0x7f)) {
    case PciDevice:
      CapabilityPtr = ConfigSpace->NonCommon.Device.CapabilityPtr;
      break;
    case PciP2pBridge:
      CapabilityPtr = ConfigSpace->NonCommon.Bridge.CapabilityPtr;
      break;
    case PciCardBusBridge:
      CapabilityPtr = ConfigSpace->NonCommon.CardBus.Cap_Ptr;
      break;
    default:
      return 0;
  }

  while ((CapabilityPtr >= 0x40) && ((CapabilityPtr & 0x03) == 0x00)) {
    CapabilityEntry = (EFI_PCI_CAPABILITY_HDR *) ((UINT8 *) ConfigSpace + CapabilityPtr);
    if (CapabilityEntry->CapabilityID == CapabilityId) {
      return CapabilityPtr;
    }

    //
    // Certain PCI device may incorrectly have capability pointing to itself,
    // break to avoid dead loop.
    //
    if (CapabilityPtr == CapabilityEntry->NextItemPtr) {
      break;
    }

    CapabilityPtr = CapabilityEntry->NextItemPtr;
  }

  return 0;
}

/**
  Print out information of the capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieCapReg (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  CHAR16 *DevicePortType;

  ShellPrintEx (-1, -1,
    L"  Capability Version(3:0):          %E0x%04x%N\r\n",
    PciExpressCap->Capability.Bits.Version
   );
  if (PciExpressCap->Capability.Bits.DevicePortType < ARRAY_SIZE (DevicePortTypeTable)) {
    DevicePortType = DevicePortTypeTable[PciExpressCap->Capability.Bits.DevicePortType];
  } else {
    DevicePortType = L"Unknown Type";
  }
  ShellPrintEx (-1, -1,
    L"  Device/PortType(7:4):             %E%s%N\r\n",
    DevicePortType
   );
  //
  // 'Slot Implemented' is only valid for:
  // a) Root Port of PCI Express Root Complex, or
  // b) Downstream Port of PCI Express Switch
  //
  if (PciExpressCap->Capability.Bits.DevicePortType== PCIE_DEVICE_PORT_TYPE_ROOT_PORT ||
      PciExpressCap->Capability.Bits.DevicePortType == PCIE_DEVICE_PORT_TYPE_DOWNSTREAM_PORT) {
    ShellPrintEx (-1, -1,
      L"  Slot Implemented(8):              %E%d%N\r\n",
      PciExpressCap->Capability.Bits.SlotImplemented
     );
  }
  ShellPrintEx (-1, -1,
    L"  Interrupt Message Number(13:9):   %E0x%05x%N\r\n",
    PciExpressCap->Capability.Bits.InterruptMessageNumber
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  UINT8  DevicePortType;
  UINT8  L0sLatency;
  UINT8  L1Latency;

  DevicePortType = (UINT8)PciExpressCap->Capability.Bits.DevicePortType;
  ShellPrintEx (-1, -1, L"  Max_Payload_Size Supported(2:0):          ");
  if (PciExpressCap->DeviceCapability.Bits.MaxPayloadSize < 6) {
    ShellPrintEx (-1, -1, L"%E%d bytes%N\r\n", 1 << (PciExpressCap->DeviceCapability.Bits.MaxPayloadSize + 7));
  } else {
    ShellPrintEx (-1, -1, L"%EUnknown%N\r\n");
  }
  ShellPrintEx (-1, -1,
    L"  Phantom Functions Supported(4:3):         %E%d%N\r\n",
    PciExpressCap->DeviceCapability.Bits.PhantomFunctions
   );
  ShellPrintEx (-1, -1,
    L"  Extended Tag Field Supported(5):          %E%d-bit Tag field supported%N\r\n",
    PciExpressCap->DeviceCapability.Bits.ExtendedTagField ? 8 : 5
   );
  //
  // Endpoint L0s and L1 Acceptable Latency is only valid for Endpoint
  //
  if (IS_PCIE_ENDPOINT (DevicePortType)) {
    L0sLatency = (UINT8)PciExpressCap->DeviceCapability.Bits.EndpointL0sAcceptableLatency;
    L1Latency  = (UINT8)PciExpressCap->DeviceCapability.Bits.EndpointL1AcceptableLatency;
    ShellPrintEx (-1, -1, L"  Endpoint L0s Acceptable Latency(8:6):     ");
    if (L0sLatency < 4) {
      ShellPrintEx (-1, -1, L"%EMaximum of %d ns%N\r\n", 1 << (L0sLatency + 6));
    } else {
      if (L0sLatency < 7) {
        ShellPrintEx (-1, -1, L"%EMaximum of %d us%N\r\n", 1 << (L0sLatency - 3));
      } else {
        ShellPrintEx (-1, -1, L"%ENo limit%N\r\n");
      }
    }
    ShellPrintEx (-1, -1, L"  Endpoint L1 Acceptable Latency(11:9):     ");
    if (L1Latency < 7) {
      ShellPrintEx (-1, -1, L"%EMaximum of %d us%N\r\n", 1 << (L1Latency + 1));
    } else {
      ShellPrintEx (-1, -1, L"%ENo limit%N\r\n");
    }
  }
  ShellPrintEx (-1, -1,
    L"  Role-based Error Reporting(15):           %E%d%N\r\n",
    PciExpressCap->DeviceCapability.Bits.RoleBasedErrorReporting
   );
  //
  // Only valid for Upstream Port:
  // a) Captured Slot Power Limit Value
  // b) Captured Slot Power Scale
  //
  if (DevicePortType == PCIE_DEVICE_PORT_TYPE_UPSTREAM_PORT) {
    ShellPrintEx (-1, -1,
      L"  Captured Slot Power Limit Value(25:18):   %E0x%02x%N\r\n",
      PciExpressCap->DeviceCapability.Bits.CapturedSlotPowerLimitValue
     );
    ShellPrintEx (-1, -1,
      L"  Captured Slot Power Limit Scale(27:26):   %E%s%N\r\n",
      SlotPwrLmtScaleTable[PciExpressCap->DeviceCapability.Bits.CapturedSlotPowerLimitScale]
     );
  }
  //
  // Function Level Reset Capability is only valid for Endpoint
  //
  if (IS_PCIE_ENDPOINT (DevicePortType)) {
    ShellPrintEx (-1, -1,
      L"  Function Level Reset Capability(28):      %E%d%N\r\n",
      PciExpressCap->DeviceCapability.Bits.FunctionLevelReset
     );
  }
  return EFI_SUCCESS;
}

/**
  Print out information of the device control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  Correctable Error Reporting Enable(0):    %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.CorrectableError
    );
  ShellPrintEx (-1, -1,
    L"  Non-Fatal Error Reporting Enable(1):      %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.NonFatalError
   );
  ShellPrintEx (-1, -1,
    L"  Fatal Error Reporting Enable(2):          %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.FatalError
   );
  ShellPrintEx (-1, -1,
    L"  Unsupported Request Reporting Enable(3):  %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.UnsupportedRequest
   );
  ShellPrintEx (-1, -1,
    L"  Enable Relaxed Ordering(4):               %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.RelaxedOrdering
   );
  ShellPrintEx (-1, -1, L"  Max_Payload_Size(7:5):                    ");
  if (PciExpressCap->DeviceControl.Bits.MaxPayloadSize < 6) {
    ShellPrintEx (-1, -1, L"%E%d bytes%N\r\n", 1 << (PciExpressCap->DeviceControl.Bits.MaxPayloadSize + 7));
  } else {
    ShellPrintEx (-1, -1, L"%EUnknown%N\r\n");
  }
  ShellPrintEx (-1, -1,
    L"  Extended Tag Field Enable(8):             %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.ExtendedTagField
   );
  ShellPrintEx (-1, -1,
    L"  Phantom Functions Enable(9):              %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.PhantomFunctions
   );
  ShellPrintEx (-1, -1,
    L"  Auxiliary (AUX) Power PM Enable(10):      %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.AuxPower
   );
  ShellPrintEx (-1, -1,
    L"  Enable No Snoop(11):                      %E%d%N\r\n",
    PciExpressCap->DeviceControl.Bits.NoSnoop
   );
  ShellPrintEx (-1, -1, L"  Max_Read_Request_Size(14:12):             ");
  if (PciExpressCap->DeviceControl.Bits.MaxReadRequestSize < 6) {
    ShellPrintEx (-1, -1, L"%E%d bytes%N\r\n", 1 << (PciExpressCap->DeviceControl.Bits.MaxReadRequestSize + 7));
  } else {
    ShellPrintEx (-1, -1, L"%EUnknown%N\r\n");
  }
  //
  // Read operation is only valid for PCI Express to PCI/PCI-X Bridges
  //
  if (PciExpressCap->Capability.Bits.DevicePortType == PCIE_DEVICE_PORT_TYPE_PCIE_TO_PCI_BRIDGE) {
    ShellPrintEx (-1, -1,
      L"  Bridge Configuration Retry Enable(15):  %E%d%N\r\n",
      PciExpressCap->DeviceControl.Bits.BridgeConfigurationRetryOrFunctionLevelReset
     );
  }
  return EFI_SUCCESS;
}

/**
  Print out information of the device status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieDeviceStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  Correctable Error Detected(0):            %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.CorrectableError
   );
  ShellPrintEx (-1, -1,
    L"  Non-Fatal Error Detected(1):              %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.NonFatalError
   );
  ShellPrintEx (-1, -1,
    L"  Fatal Error Detected(2):                  %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.FatalError
   );
  ShellPrintEx (-1, -1,
    L"  Unsupported Request Detected(3):          %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.UnsupportedRequest
   );
  ShellPrintEx (-1, -1,
    L"  AUX Power Detected(4):                    %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.AuxPower
   );
  ShellPrintEx (-1, -1,
    L"  Transactions Pending(5):                  %E%d%N\r\n",
    PciExpressCap->DeviceStatus.Bits.TransactionsPending
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device link information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  CHAR16 *MaxLinkSpeed;
  CHAR16 *AspmValue;

  switch (PciExpressCap->LinkCapability.Bits.MaxLinkSpeed) {
    case 1:
      MaxLinkSpeed = L"2.5 GT/s";
      break;
    case 2:
      MaxLinkSpeed = L"5.0 GT/s";
      break;
    case 3:
      MaxLinkSpeed = L"8.0 GT/s";
      break;
    case 4:
      MaxLinkSpeed = L"16.0 GT/s";
      break;
    case 5:
      MaxLinkSpeed = L"32.0 GT/s";
      break;
    default:
      MaxLinkSpeed = L"Reserved";
      break;
  }
  ShellPrintEx (-1, -1,
    L"  Maximum Link Speed(3:0):                            %E%s%N\r\n",
    MaxLinkSpeed
   );
  ShellPrintEx (-1, -1,
    L"  Maximum Link Width(9:4):                            %Ex%d%N\r\n",
    PciExpressCap->LinkCapability.Bits.MaxLinkWidth
   );
  switch (PciExpressCap->LinkCapability.Bits.Aspm) {
    case 0:
      AspmValue = L"Not";
      break;
    case 1:
      AspmValue = L"L0s";
      break;
    case 2:
      AspmValue = L"L1";
      break;
    case 3:
      AspmValue = L"L0s and L1";
      break;
    default:
      AspmValue = L"Reserved";
      break;
  }
  ShellPrintEx (-1, -1,
    L"  Active State Power Management Support(11:10):       %E%s Supported%N\r\n",
    AspmValue
   );
  ShellPrintEx (-1, -1,
    L"  L0s Exit Latency(14:12):                            %E%s%N\r\n",
    L0sLatencyStrTable[PciExpressCap->LinkCapability.Bits.L0sExitLatency]
   );
  ShellPrintEx (-1, -1,
    L"  L1 Exit Latency(17:15):                             %E%s%N\r\n",
    L1LatencyStrTable[PciExpressCap->LinkCapability.Bits.L1ExitLatency]
   );
  ShellPrintEx (-1, -1,
    L"  Clock Power Management(18):                         %E%d%N\r\n",
    PciExpressCap->LinkCapability.Bits.ClockPowerManagement
   );
  ShellPrintEx (-1, -1,
    L"  Surprise Down Error Reporting Capable(19):          %E%d%N\r\n",
    PciExpressCap->LinkCapability.Bits.SurpriseDownError
   );
  ShellPrintEx (-1, -1,
    L"  Data Link Layer Link Active Reporting Capable(20):  %E%d%N\r\n",
    PciExpressCap->LinkCapability.Bits.DataLinkLayerLinkActive
   );
  ShellPrintEx (-1, -1,
    L"  Link Bandwidth Notification Capability(21):         %E%d%N\r\n",
    PciExpressCap->LinkCapability.Bits.LinkBandwidthNotification
   );
  ShellPrintEx (-1, -1,
    L"  Port Number(31:24):                                 %E0x%02x%N\r\n",
    PciExpressCap->LinkCapability.Bits.PortNumber
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device link control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  UINT8  DevicePortType;

  DevicePortType  = (UINT8)PciExpressCap->Capability.Bits.DevicePortType;
  ShellPrintEx (-1, -1,
    L"  Active State Power Management Control(1:0):         %E%s%N\r\n",
    ASPMCtrlStrTable[PciExpressCap->LinkControl.Bits.AspmControl]
   );
  //
  // RCB is not applicable to switches
  //
  if (!IS_PCIE_SWITCH(DevicePortType)) {
    ShellPrintEx (-1, -1,
      L"  Read Completion Boundary (RCB)(3):                  %E%d byte%N\r\n",
      1 << (PciExpressCap->LinkControl.Bits.ReadCompletionBoundary + 6)
     );
  }
  //
  // Link Disable is reserved on
  // a) Endpoints
  // b) PCI Express to PCI/PCI-X bridges
  // c) Upstream Ports of Switches
  //
  if (!IS_PCIE_ENDPOINT (DevicePortType) &&
      DevicePortType != PCIE_DEVICE_PORT_TYPE_UPSTREAM_PORT &&
      DevicePortType != PCIE_DEVICE_PORT_TYPE_PCIE_TO_PCI_BRIDGE) {
    ShellPrintEx (-1, -1,
      L"  Link Disable(4):                                    %E%d%N\r\n",
      PciExpressCap->LinkControl.Bits.LinkDisable
     );
  }
  ShellPrintEx (-1, -1,
    L"  Common Clock Configuration(6):                      %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.CommonClockConfiguration
   );
  ShellPrintEx (-1, -1,
    L"  Extended Synch(7):                                  %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.ExtendedSynch
   );
  ShellPrintEx (-1, -1,
    L"  Enable Clock Power Management(8):                   %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.ClockPowerManagement
   );
  ShellPrintEx (-1, -1,
    L"  Hardware Autonomous Width Disable(9):               %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.HardwareAutonomousWidthDisable
   );
  ShellPrintEx (-1, -1,
    L"  Link Bandwidth Management Interrupt Enable(10):     %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.LinkBandwidthManagementInterrupt
   );
  ShellPrintEx (-1, -1,
    L"  Link Autonomous Bandwidth Interrupt Enable(11):     %E%d%N\r\n",
    PciExpressCap->LinkControl.Bits.LinkAutonomousBandwidthInterrupt
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device link status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieLinkStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  CHAR16 *CurLinkSpeed;

  switch (PciExpressCap->LinkStatus.Bits.CurrentLinkSpeed) {
    case 1:
      CurLinkSpeed = L"2.5 GT/s";
      break;
    case 2:
      CurLinkSpeed = L"5.0 GT/s";
      break;
    case 3:
      CurLinkSpeed = L"8.0 GT/s";
      break;
    case 4:
      CurLinkSpeed = L"16.0 GT/s";
      break;
    case 5:
      CurLinkSpeed = L"32.0 GT/s";
      break;
    default:
      CurLinkSpeed = L"Reserved";
      break;
  }
  ShellPrintEx (-1, -1,
    L"  Current Link Speed(3:0):                            %E%s%N\r\n",
    CurLinkSpeed
   );
  ShellPrintEx (-1, -1,
    L"  Negotiated Link Width(9:4):                         %Ex%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.NegotiatedLinkWidth
   );
  ShellPrintEx (-1, -1,
    L"  Link Training(11):                                  %E%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.LinkTraining
   );
  ShellPrintEx (-1, -1,
    L"  Slot Clock Configuration(12):                       %E%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.SlotClockConfiguration
   );
  ShellPrintEx (-1, -1,
    L"  Data Link Layer Link Active(13):                    %E%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.DataLinkLayerLinkActive
   );
  ShellPrintEx (-1, -1,
    L"  Link Bandwidth Management Status(14):               %E%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.LinkBandwidthManagement
   );
  ShellPrintEx (-1, -1,
    L"  Link Autonomous Bandwidth Status(15):               %E%d%N\r\n",
    PciExpressCap->LinkStatus.Bits.LinkAutonomousBandwidth
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device slot information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  Attention Button Present(0):                        %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.AttentionButton
   );
  ShellPrintEx (-1, -1,
    L"  Power Controller Present(1):                        %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.PowerController
   );
  ShellPrintEx (-1, -1,
    L"  MRL Sensor Present(2):                              %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.MrlSensor
   );
  ShellPrintEx (-1, -1,
    L"  Attention Indicator Present(3):                     %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.AttentionIndicator
   );
  ShellPrintEx (-1, -1,
    L"  Power Indicator Present(4):                         %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.PowerIndicator
   );
  ShellPrintEx (-1, -1,
    L"  Hot-Plug Surprise(5):                               %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.HotPlugSurprise
   );
  ShellPrintEx (-1, -1,
    L"  Hot-Plug Capable(6):                                %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.HotPlugCapable
   );
  ShellPrintEx (-1, -1,
    L"  Slot Power Limit Value(14:7):                       %E0x%02x%N\r\n",
    PciExpressCap->SlotCapability.Bits.SlotPowerLimitValue
   );
  ShellPrintEx (-1, -1,
    L"  Slot Power Limit Scale(16:15):                      %E%s%N\r\n",
    SlotPwrLmtScaleTable[PciExpressCap->SlotCapability.Bits.SlotPowerLimitScale]
   );
  ShellPrintEx (-1, -1,
    L"  Electromechanical Interlock Present(17):            %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.ElectromechanicalInterlock
   );
  ShellPrintEx (-1, -1,
    L"  No Command Completed Support(18):                   %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.NoCommandCompleted
   );
  ShellPrintEx (-1, -1,
    L"  Physical Slot Number(31:19):                        %E%d%N\r\n",
    PciExpressCap->SlotCapability.Bits.PhysicalSlotNumber
   );

  return EFI_SUCCESS;
}

/**
  Print out information of the device slot control information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  Attention Button Pressed Enable(0):                 %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.AttentionButtonPressed
   );
  ShellPrintEx (-1, -1,
    L"  Power Fault Detected Enable(1):                     %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.PowerFaultDetected
   );
  ShellPrintEx (-1, -1,
    L"  MRL Sensor Changed Enable(2):                       %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.MrlSensorChanged
   );
  ShellPrintEx (-1, -1,
    L"  Presence Detect Changed Enable(3):                  %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.PresenceDetectChanged
   );
  ShellPrintEx (-1, -1,
    L"  Command Completed Interrupt Enable(4):              %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.CommandCompletedInterrupt
   );
  ShellPrintEx (-1, -1,
    L"  Hot-Plug Interrupt Enable(5):                       %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.HotPlugInterrupt
   );
  ShellPrintEx (-1, -1,
    L"  Attention Indicator Control(7:6):                   %E%s%N\r\n",
    IndicatorTable[
    PciExpressCap->SlotControl.Bits.AttentionIndicator]
   );
  ShellPrintEx (-1, -1,
    L"  Power Indicator Control(9:8):                       %E%s%N\r\n",
    IndicatorTable[PciExpressCap->SlotControl.Bits.PowerIndicator]
   );
  ShellPrintEx (-1, -1, L"  Power Controller Control(10):                       %EPower ");
  if (
    PciExpressCap->SlotControl.Bits.PowerController) {
    ShellPrintEx (-1, -1, L"Off%N\r\n");
  } else {
    ShellPrintEx (-1, -1, L"On%N\r\n");
  }
  ShellPrintEx (-1, -1,
    L"  Electromechanical Interlock Control(11):            %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.ElectromechanicalInterlock
   );
  ShellPrintEx (-1, -1,
    L"  Data Link Layer State Changed Enable(12):           %E%d%N\r\n",
    PciExpressCap->SlotControl.Bits.DataLinkLayerStateChanged
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device slot status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieSlotStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  Attention Button Pressed(0):           %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.AttentionButtonPressed
   );
  ShellPrintEx (-1, -1,
    L"  Power Fault Detected(1):               %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.PowerFaultDetected
   );
  ShellPrintEx (-1, -1,
    L"  MRL Sensor Changed(2):                 %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.MrlSensorChanged
   );
  ShellPrintEx (-1, -1,
    L"  Presence Detect Changed(3):            %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.PresenceDetectChanged
   );
  ShellPrintEx (-1, -1,
    L"  Command Completed(4):                  %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.CommandCompleted
   );
  ShellPrintEx (-1, -1, L"  MRL Sensor State(5):                   %EMRL ");
  if (
    PciExpressCap->SlotStatus.Bits.MrlSensor) {
    ShellPrintEx (-1, -1, L" Opened%N\r\n");
  } else {
    ShellPrintEx (-1, -1, L" Closed%N\r\n");
  }
  ShellPrintEx (-1, -1, L"  Presence Detect State(6):              ");
  if (
    PciExpressCap->SlotStatus.Bits.PresenceDetect) {
    ShellPrintEx (-1, -1, L"%ECard Present in slot%N\r\n");
  } else {
    ShellPrintEx (-1, -1, L"%ESlot Empty%N\r\n");
  }
  ShellPrintEx (-1, -1, L"  Electromechanical Interlock Status(7): %EElectromechanical Interlock ");
  if (
    PciExpressCap->SlotStatus.Bits.ElectromechanicalInterlock) {
    ShellPrintEx (-1, -1, L"Engaged%N\r\n");
  } else {
    ShellPrintEx (-1, -1, L"Disengaged%N\r\n");
  }
  ShellPrintEx (-1, -1,
    L"  Data Link Layer State Changed(8):      %E%d%N\r\n",
    PciExpressCap->SlotStatus.Bits.DataLinkLayerStateChanged
   );
  return EFI_SUCCESS;
}

/**
  Print out information of the device root information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootControl (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  System Error on Correctable Error Enable(0):  %E%d%N\r\n",
    PciExpressCap->RootControl.Bits.SystemErrorOnCorrectableError
   );
  ShellPrintEx (-1, -1,
    L"  System Error on Non-Fatal Error Enable(1):    %E%d%N\r\n",
    PciExpressCap->RootControl.Bits.SystemErrorOnNonFatalError
   );
  ShellPrintEx (-1, -1,
    L"  System Error on Fatal Error Enable(2):        %E%d%N\r\n",
    PciExpressCap->RootControl.Bits.SystemErrorOnFatalError
   );
  ShellPrintEx (-1, -1,
    L"  PME Interrupt Enable(3):                      %E%d%N\r\n",
    PciExpressCap->RootControl.Bits.PmeInterrupt
   );
  ShellPrintEx (-1, -1,
    L"  CRS Software Visibility Enable(4):            %E%d%N\r\n",
    PciExpressCap->RootControl.Bits.CrsSoftwareVisibility
   );

  return EFI_SUCCESS;
}

/**
  Print out information of the device root capability information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootCap (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  CRS Software Visibility(0):                   %E%d%N\r\n",
    PciExpressCap->RootCapability.Bits.CrsSoftwareVisibility
   );

  return EFI_SUCCESS;
}

/**
  Print out information of the device root status information.

  @param[in] PciExpressCap  The pointer to the structure about the device.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ExplainPcieRootStatus (
  IN PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  ShellPrintEx (-1, -1,
    L"  PME Requester ID(15:0):                       %E0x%04x%N\r\n",
    PciExpressCap->RootStatus.Bits.PmeRequesterId
   );
  ShellPrintEx (-1, -1,
    L"  PME Status(16):                               %E%d%N\r\n",
    PciExpressCap->RootStatus.Bits.PmeStatus
   );
  ShellPrintEx (-1, -1,
    L"  PME Pending(17):                              %E%d%N\r\n",
    PciExpressCap->RootStatus.Bits.PmePending
   );
  return EFI_SUCCESS;
}

/**
  Function to interpret and print out the link control structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityLinkControl (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_INTERNAL_LINK_CONTROL *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_INTERNAL_LINK_CONTROL*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_LINK_CONTROL),
    gShellDebug1HiiHandle,
    Header->RootComplexLinkCapabilities,
    Header->RootComplexLinkControl,
    Header->RootComplexLinkStatus
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_INTERNAL_LINK_CONTROL),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the power budgeting structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityPowerBudgeting (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_POWER_BUDGETING *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_POWER_BUDGETING*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_POWER),
    gShellDebug1HiiHandle,
    Header->DataSelect,
    Header->Data,
    Header->PowerBudgetCapability
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_POWER_BUDGETING),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the ACS structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityAcs (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_ACS_EXTENDED  *Header;
  UINT16                                                VectorSize;
  UINT16                                                LoopCounter;

  Header      = (PCI_EXPRESS_EXTENDED_CAPABILITIES_ACS_EXTENDED*)HeaderAddress;
  VectorSize  = 0;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_ACS),
    gShellDebug1HiiHandle,
    Header->AcsCapability,
    Header->AcsControl
    );
  if (PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_GET_EGRES_CONTROL(Header)) {
    VectorSize = PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_GET_EGRES_VECTOR_SIZE(Header);
    if (VectorSize == 0) {
      VectorSize = 256;
    }
    for (LoopCounter = 0 ; LoopCounter * 8 < VectorSize ; LoopCounter++) {
      ShellPrintHiiEx(
        -1, -1, NULL,
        STRING_TOKEN (STR_PCI_EXT_CAP_ACS2),
        gShellDebug1HiiHandle,
        LoopCounter + 1,
        Header->EgressControlVectorArray[LoopCounter]
        );
    }
  }
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_ACS_EXTENDED) + (VectorSize / 8) - 1,
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the latency tolerance reporting structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityLatencyToleranceReporting (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_LATENCE_TOLERANCE_REPORTING *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_LATENCE_TOLERANCE_REPORTING*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_LAT),
    gShellDebug1HiiHandle,
    Header->MaxSnoopLatency,
    Header->MaxNoSnoopLatency
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_LATENCE_TOLERANCE_REPORTING),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the serial number structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilitySerialNumber (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_SERIAL_NUMBER *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_SERIAL_NUMBER*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_SN),
    gShellDebug1HiiHandle,
    Header->SerialNumber
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_SERIAL_NUMBER),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the RCRB structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityRcrb (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_RCRB_HEADER *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_RCRB_HEADER*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_RCRB),
    gShellDebug1HiiHandle,
    Header->VendorId,
    Header->DeviceId,
    Header->RcrbCapabilities,
    Header->RcrbControl
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_RCRB_HEADER),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the vendor specific structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityVendorSpecific (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_VENDOR_SPECIFIC *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_VENDOR_SPECIFIC*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_VEN),
    gShellDebug1HiiHandle,
    Header->VendorSpecificHeader
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    PCI_EXPRESS_EXTENDED_CAPABILITY_VENDOR_SPECIFIC_GET_SIZE(Header),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the Event Collector Endpoint Association structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityECEA (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_ECEA),
    gShellDebug1HiiHandle,
    Header->AssociationBitmap
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the ARI structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityAri (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_ARI_CAPABILITY *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_ARI_CAPABILITY*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_ARI),
    gShellDebug1HiiHandle,
    Header->AriCapability,
    Header->AriControl
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_ARI_CAPABILITY),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the DPA structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityDynamicPowerAllocation (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_DYNAMIC_POWER_ALLOCATION *Header;
  UINT8                                                            LinkCount;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_DYNAMIC_POWER_ALLOCATION*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_DPA),
    gShellDebug1HiiHandle,
    Header->DpaCapability,
    Header->DpaLatencyIndicator,
    Header->DpaStatus,
    Header->DpaControl
    );
  for (LinkCount = 0 ; LinkCount < PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_GET_SUBSTATE_MAX(Header) + 1 ; LinkCount++) {
    ShellPrintHiiEx(
      -1, -1, NULL,
      STRING_TOKEN (STR_PCI_EXT_CAP_DPA2),
      gShellDebug1HiiHandle,
      LinkCount+1,
      Header->DpaPowerAllocationArray[LinkCount]
      );
  }
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_DYNAMIC_POWER_ALLOCATION) - 1 + PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_GET_SUBSTATE_MAX(Header),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the link declaration structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityLinkDeclaration (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_LINK_DECLARATION  *Header;
  UINT8                                                     LinkCount;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_LINK_DECLARATION*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_LINK_DECLAR),
    gShellDebug1HiiHandle,
    Header->ElementSelfDescription
    );

  for (LinkCount = 0 ; LinkCount < PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_GET_LINK_COUNT(Header) ; LinkCount++) {
    ShellPrintHiiEx(
      -1, -1, NULL,
      STRING_TOKEN (STR_PCI_EXT_CAP_LINK_DECLAR2),
      gShellDebug1HiiHandle,
      LinkCount+1,
      Header->LinkEntry[LinkCount]
      );
  }
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_LINK_DECLARATION) + (PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_GET_LINK_COUNT(Header)-1)*sizeof(UINT32),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the Advanced Error Reporting structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityAer (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_ADVANCED_ERROR_REPORTING *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_ADVANCED_ERROR_REPORTING*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_AER),
    gShellDebug1HiiHandle,
    Header->UncorrectableErrorStatus,
    Header->UncorrectableErrorMask,
    Header->UncorrectableErrorSeverity,
    Header->CorrectableErrorStatus,
    Header->CorrectableErrorMask,
    Header->AdvancedErrorCapabilitiesAndControl,
    Header->HeaderLog[0],
    Header->HeaderLog[1],
    Header->HeaderLog[2],
    Header->HeaderLog[3],
    Header->RootErrorCommand,
    Header->RootErrorStatus,
    Header->ErrorSourceIdentification,
    Header->CorrectableErrorSourceIdentification,
    Header->TlpPrefixLog[0],
    Header->TlpPrefixLog[1],
    Header->TlpPrefixLog[2],
    Header->TlpPrefixLog[3]
    );
  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_ADVANCED_ERROR_REPORTING),
    (VOID *) (HeaderAddress)
    );
  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the multicast structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
  @param[in] PciExpressCapPtr     The address of the PCIe capabilities structure.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityMulticast (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress,
  IN CONST PCI_CAPABILITY_PCIEXP *PciExpressCapPtr
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_MULTICAST *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_MULTICAST*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_MULTICAST),
    gShellDebug1HiiHandle,
    Header->MultiCastCapability,
    Header->MulticastControl,
    Header->McBaseAddress,
    Header->McReceiveAddress,
    Header->McBlockAll,
    Header->McBlockUntranslated,
    Header->McOverlayBar
    );

  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_MULTICAST),
    (VOID *) (HeaderAddress)
    );

  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the virtual channel and multi virtual channel structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityVirtualChannel (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_CAPABILITY  *Header;
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_VC          *CapabilityItem;
  UINT32                                                              ItemCount;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_CAPABILITY*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_VC_BASE),
    gShellDebug1HiiHandle,
    Header->ExtendedVcCount,
    Header->PortVcCapability1,
    Header->PortVcCapability2,
    Header->VcArbTableOffset,
    Header->PortVcControl,
    Header->PortVcStatus
    );
  for (ItemCount = 0 ; ItemCount < Header->ExtendedVcCount ; ItemCount++) {
    CapabilityItem = &Header->Capability[ItemCount];
    ShellPrintHiiEx(
      -1, -1, NULL,
      STRING_TOKEN (STR_PCI_EXT_CAP_VC_ITEM),
      gShellDebug1HiiHandle,
      ItemCount+1,
      CapabilityItem->VcResourceCapability,
      CapabilityItem->PortArbTableOffset,
      CapabilityItem->VcResourceControl,
      CapabilityItem->VcResourceStatus
      );
  }

  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_CAPABILITY)
    + Header->ExtendedVcCount * sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_VIRTUAL_CHANNEL_VC),
    (VOID *) (HeaderAddress)
    );

  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the resizeable bar structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityResizeableBar (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR        *Header;
  UINT32                                                       ItemCount;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR*)HeaderAddress;

  for (ItemCount = 0 ; ItemCount < (UINT32)GET_NUMBER_RESIZABLE_BARS(Header) ; ItemCount++) {
    ShellPrintHiiEx(
      -1, -1, NULL,
      STRING_TOKEN (STR_PCI_EXT_CAP_RESIZE_BAR),
      gShellDebug1HiiHandle,
      ItemCount+1,
      Header->Capability[ItemCount].ResizableBarCapability.Uint32,
      Header->Capability[ItemCount].ResizableBarControl.Uint32
      );
  }

  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    (UINT32)GET_NUMBER_RESIZABLE_BARS(Header) * sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_RESIZABLE_BAR_ENTRY),
    (VOID *) (HeaderAddress)
    );

  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the TPH structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilityTph (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_TPH *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_TPH*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_TPH),
    gShellDebug1HiiHandle,
    Header->TphRequesterCapability,
    Header->TphRequesterControl
    );
  DumpHex (
    8,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)Header->TphStTable - (UINT8*)HeadersBaseAddress),
    GET_TPH_TABLE_SIZE(Header),
    (VOID *)Header->TphStTable
    );

  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof(PCI_EXPRESS_EXTENDED_CAPABILITIES_TPH) + GET_TPH_TABLE_SIZE(Header) - sizeof(UINT16),
    (VOID *) (HeaderAddress)
    );

  return (EFI_SUCCESS);
}

/**
  Function to interpret and print out the secondary PCIe capability structure

  @param[in] HeaderAddress        The Address of this capability header.
  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
  @param[in] PciExpressCapPtr     The address of the PCIe capabilities structure.
**/
EFI_STATUS
PrintInterpretedExtendedCompatibilitySecondary (
  IN CONST PCI_EXP_EXT_HDR *HeaderAddress,
  IN CONST PCI_EXP_EXT_HDR *HeadersBaseAddress,
  IN CONST PCI_CAPABILITY_PCIEXP *PciExpressCap
  )
{
  CONST PCI_EXPRESS_EXTENDED_CAPABILITIES_SECONDARY_PCIE *Header;
  Header = (PCI_EXPRESS_EXTENDED_CAPABILITIES_SECONDARY_PCIE*)HeaderAddress;

  ShellPrintHiiEx(
    -1, -1, NULL,
    STRING_TOKEN (STR_PCI_EXT_CAP_SECONDARY),
    gShellDebug1HiiHandle,
    Header->LinkControl3.Uint32,
    Header->LaneErrorStatus
    );
  DumpHex (
    8,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)Header->EqualizationControl - (UINT8*)HeadersBaseAddress),
    PciExpressCap->LinkCapability.Bits.MaxLinkWidth * sizeof (PCI_EXPRESS_REG_LANE_EQUALIZATION_CONTROL),
    (VOID *)Header->EqualizationControl
    );

  DumpHex (
    4,
    EFI_PCIE_CAPABILITY_BASE_OFFSET + ((UINT8*)HeaderAddress - (UINT8*)HeadersBaseAddress),
    sizeof (PCI_EXPRESS_EXTENDED_CAPABILITIES_SECONDARY_PCIE) - sizeof (Header->EqualizationControl)
      + PciExpressCap->LinkCapability.Bits.MaxLinkWidth * sizeof (PCI_EXPRESS_REG_LANE_EQUALIZATION_CONTROL),
    (VOID *) (HeaderAddress)
    );

  return (EFI_SUCCESS);
}

/**
  Display Pcie extended capability details

  @param[in] HeadersBaseAddress   The address of all the extended capability headers.
  @param[in] HeaderAddress        The address of this capability header.
  @param[in] PciExpressCapPtr     The address of the PCIe capabilities structure.
**/
EFI_STATUS
PrintPciExtendedCapabilityDetails(
  IN CONST PCI_EXP_EXT_HDR    *HeadersBaseAddress,
  IN CONST PCI_EXP_EXT_HDR    *HeaderAddress,
  IN CONST PCI_CAPABILITY_PCIEXP *PciExpressCapPtr
  )
{
  switch (HeaderAddress->CapabilityId){
    case PCI_EXPRESS_EXTENDED_CAPABILITY_ADVANCED_ERROR_REPORTING_ID:
      return PrintInterpretedExtendedCompatibilityAer(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_CONTROL_ID:
      return PrintInterpretedExtendedCompatibilityLinkControl(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_LINK_DECLARATION_ID:
      return PrintInterpretedExtendedCompatibilityLinkDeclaration(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_SERIAL_NUMBER_ID:
      return PrintInterpretedExtendedCompatibilitySerialNumber(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_POWER_BUDGETING_ID:
      return PrintInterpretedExtendedCompatibilityPowerBudgeting(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_ACS_EXTENDED_ID:
      return PrintInterpretedExtendedCompatibilityAcs(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_LATENCE_TOLERANCE_REPORTING_ID:
      return PrintInterpretedExtendedCompatibilityLatencyToleranceReporting(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_ARI_CAPABILITY_ID:
      return PrintInterpretedExtendedCompatibilityAri(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_RCRB_HEADER_ID:
      return PrintInterpretedExtendedCompatibilityRcrb(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_VENDOR_SPECIFIC_ID:
      return PrintInterpretedExtendedCompatibilityVendorSpecific(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_DYNAMIC_POWER_ALLOCATION_ID:
      return PrintInterpretedExtendedCompatibilityDynamicPowerAllocation(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_EVENT_COLLECTOR_ENDPOINT_ASSOCIATION_ID:
      return PrintInterpretedExtendedCompatibilityECEA(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_VIRTUAL_CHANNEL_ID:
    case PCI_EXPRESS_EXTENDED_CAPABILITY_MULTI_FUNCTION_VIRTUAL_CHANNEL_ID:
      return PrintInterpretedExtendedCompatibilityVirtualChannel(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_MULTICAST_ID:
      //
      // should only be present if PCIE_CAP_DEVICEPORT_TYPE(PciExpressCapPtr->PcieCapReg) == 0100b, 0101b, or 0110b
      //
      return PrintInterpretedExtendedCompatibilityMulticast(HeaderAddress, HeadersBaseAddress, PciExpressCapPtr);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_RESIZABLE_BAR_ID:
      return PrintInterpretedExtendedCompatibilityResizeableBar(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_TPH_ID:
      return PrintInterpretedExtendedCompatibilityTph(HeaderAddress, HeadersBaseAddress);
    case PCI_EXPRESS_EXTENDED_CAPABILITY_SECONDARY_PCIE_ID:
      return PrintInterpretedExtendedCompatibilitySecondary(HeaderAddress, HeadersBaseAddress, PciExpressCapPtr);
    default:
      ShellPrintEx (-1, -1,
        L"Unknown PCIe extended capability ID (%04xh).  No interpretation available.\r\n",
        HeaderAddress->CapabilityId
        );
      return EFI_SUCCESS;
  };

}

/**
  Display Pcie device structure.

  @param[in] PciExpressCap       PCI Express capability buffer.
  @param[in] ExtendedConfigSpace PCI Express extended configuration space.
  @param[in] ExtendedConfigSize  PCI Express extended configuration size.
  @param[in] ExtendedCapability  PCI Express extended capability ID to explain.
**/
VOID
PciExplainPciExpress (
  IN  PCI_CAPABILITY_PCIEXP                  *PciExpressCap,
  IN  UINT8                                  *ExtendedConfigSpace,
  IN  UINTN                                  ExtendedConfigSize,
  IN CONST UINT16                            ExtendedCapability
  )
{
  UINT8                 DevicePortType;
  UINTN                 Index;
  UINT8                 *RegAddr;
  UINTN                 RegValue;
  PCI_EXP_EXT_HDR       *ExtHdr;

  DevicePortType = (UINT8)PciExpressCap->Capability.Bits.DevicePortType;

  ShellPrintEx (-1, -1, L"\r\nPci Express device capability structure:\r\n");

  for (Index = 0; PcieExplainList[Index].Type < PcieExplainTypeMax; Index++) {
    if (ShellGetExecutionBreakFlag()) {
      return;
    }
    RegAddr = (UINT8 *) PciExpressCap + PcieExplainList[Index].Offset;
    switch (PcieExplainList[Index].Width) {
      case FieldWidthUINT8:
        RegValue = *(UINT8 *) RegAddr;
        break;
      case FieldWidthUINT16:
        RegValue = *(UINT16 *) RegAddr;
        break;
      case FieldWidthUINT32:
        RegValue = *(UINT32 *) RegAddr;
        break;
      default:
        RegValue = 0;
        break;
    }
    ShellPrintHiiEx(-1, -1, NULL,
      PcieExplainList[Index].Token,
      gShellDebug1HiiHandle,
      PcieExplainList[Index].Offset,
      RegValue
     );
    if (PcieExplainList[Index].Func == NULL) {
      continue;
    }
    switch (PcieExplainList[Index].Type) {
      case PcieExplainTypeLink:
        //
        // Link registers should not be used by
        // a) Root Complex Integrated Endpoint
        // b) Root Complex Event Collector
        //
        if (DevicePortType == PCIE_DEVICE_PORT_TYPE_ROOT_COMPLEX_INTEGRATED_ENDPOINT ||
            DevicePortType == PCIE_DEVICE_PORT_TYPE_ROOT_COMPLEX_EVENT_COLLECTOR) {
          continue;
        }
        break;
      case PcieExplainTypeSlot:
        //
        // Slot registers are only valid for
        // a) Root Port of PCI Express Root Complex
        // b) Downstream Port of PCI Express Switch
        // and when SlotImplemented bit is set in PCIE cap register.
        //
        if ((DevicePortType != PCIE_DEVICE_PORT_TYPE_ROOT_PORT &&
             DevicePortType != PCIE_DEVICE_PORT_TYPE_DOWNSTREAM_PORT) ||
             !PciExpressCap->Capability.Bits.SlotImplemented) {
          continue;
        }
        break;
      case PcieExplainTypeRoot:
        //
        // Root registers are only valid for
        // Root Port of PCI Express Root Complex
        //
        if (DevicePortType != PCIE_DEVICE_PORT_TYPE_ROOT_PORT) {
          continue;
        }
        break;
      default:
        break;
    }
    PcieExplainList[Index].Func (PciExpressCap);
  }

  ExtHdr = (PCI_EXP_EXT_HDR*)ExtendedConfigSpace;
  while (ExtHdr->CapabilityId != 0 && ExtHdr->CapabilityVersion != 0 && ExtHdr->CapabilityId != 0xFFFF) {
    //
    // Process this item
    //
    if (ExtendedCapability == 0xFFFF || ExtendedCapability == ExtHdr->CapabilityId) {
      //
      // Print this item
      //
      PrintPciExtendedCapabilityDetails((PCI_EXP_EXT_HDR*)ExtendedConfigSpace, ExtHdr, PciExpressCap);
    }

    //
    // Advance to the next item if it exists
    //
    if (ExtHdr->NextCapabilityOffset != 0 &&
       (ExtHdr->NextCapabilityOffset <= (UINT32) (ExtendedConfigSize + EFI_PCIE_CAPABILITY_BASE_OFFSET - sizeof (PCI_EXP_EXT_HDR)))) {
      ExtHdr = (PCI_EXP_EXT_HDR*)(ExtendedConfigSpace + ExtHdr->NextCapabilityOffset - EFI_PCIE_CAPABILITY_BASE_OFFSET);
    } else {
      break;
    }
  }
}
