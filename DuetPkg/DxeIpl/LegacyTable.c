/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  LegacyTable.c

Abstract:

Revision History:

**/

#include "DxeIpl.h"
#include "HobGeneration.h"
#include "Debug.h"

#define ACPI_RSD_PTR      0x2052545020445352LL
#define MPS_PTR           SIGNATURE_32('_','M','P','_')
#define SMBIOS_PTR        SIGNATURE_32('_','S','M','_')

#define EBDA_BASE_ADDRESS 0x40E

VOID *
FindAcpiRsdPtr (
  VOID
  )
{
  UINTN                           Address;
  UINTN                           Index;

  //
  // First Seach 0x0e0000 - 0x0fffff for RSD Ptr
  //
  for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
    if (*(UINT64 *)(Address) == ACPI_RSD_PTR) {
      return (VOID *)Address;
    }
  }

  //
  // Search EBDA
  //

  Address = (*(UINT16 *)(UINTN)(EBDA_BASE_ADDRESS)) << 4;
  for (Index = 0; Index < 0x400 ; Index += 16) {
    if (*(UINT64 *)(Address + Index) == ACPI_RSD_PTR) {
      return (VOID *)Address;
    }
  }
  return NULL;
}

VOID *
FindSMBIOSPtr (
  VOID
  )
{
  UINTN                           Address;

  //
  // First Seach 0x0f0000 - 0x0fffff for SMBIOS Ptr
  //
  for (Address = 0xf0000; Address < 0xfffff; Address += 0x10) {
    if (*(UINT32 *)(Address) == SMBIOS_PTR) {
      return (VOID *)Address;
    }
  }
  return NULL;
}

VOID *
FindMPSPtr (
  VOID
  )
{
  UINTN                           Address;
  UINTN                           Index;

  //
  // First Seach 0x0e0000 - 0x0fffff for MPS Ptr
  //
  for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
    if (*(UINT32 *)(Address) == MPS_PTR) {
      return (VOID *)Address;
    }
  }

  //
  // Search EBDA
  //

  Address = (*(UINT16 *)(UINTN)(EBDA_BASE_ADDRESS)) << 4;
  for (Index = 0; Index < 0x400 ; Index += 16) {
    if (*(UINT32 *)(Address + Index) == MPS_PTR) {
      return (VOID *)Address;
    }
  }
  return NULL;
}

#pragma pack(1)
typedef struct {
  UINT8           Signature[8];
  UINT8           Checksum;
  UINT8           OemId[6];
  UINT8           Revision;
  UINT32          RsdtAddress;
  UINT32          Length;
  UINT64          XsdtAddress;
  UINT8           ExtendedChecksum;
  UINT8           Reserved[3];
} RSDP_TABLE;

typedef struct {
  UINT32          Signature;
  UINT32          Length;
  UINT8           Revision;
  UINT8           Checksum;
  UINT8           OemId[6];
  UINT8           OemTableId[8];
  UINT32          OemRevision;
  UINT8           CreatorId[4];
  UINT32          CreatorRevision;
} DESCRIPTION_HEADER;

typedef struct {
  DESCRIPTION_HEADER    Header;
  UINT32                Entry;
} RSDT_TABLE;

typedef struct {
  DESCRIPTION_HEADER    Header;
  UINT64                Entry;
} XSDT_TABLE;

typedef struct {
  UINT8                 Address_Space_ID;
  UINT8                 Register_Bit_Width;
  UINT8                 Register_Bit_Offset;
  UINT8                 Access_Size;
  UINT64                Address;
} GADDRESS_STRUCTURE;

#pragma pack()

VOID
ScanTableInRSDT (
  RSDT_TABLE            *Rsdt,
  UINT32                Signature,
  DESCRIPTION_HEADER    **FoundTable
  )
{
  UINTN                     Index;
  UINT32                    EntryCount;
  UINT32                    *EntryPtr;
  DESCRIPTION_HEADER        *Table;
  
  *FoundTable = NULL;
  
  EntryCount = (Rsdt->Header.Length - sizeof (DESCRIPTION_HEADER)) / sizeof(UINT32);
  
  EntryPtr = &Rsdt->Entry;
  for (Index = 0; Index < EntryCount; Index ++, EntryPtr ++) {
    Table = (DESCRIPTION_HEADER*)((UINTN)(*EntryPtr));
    if (Table->Signature == Signature) {
      *FoundTable = Table;
      break;
    }
  }
  
  return;
}

VOID
ScanTableInXSDT (
  XSDT_TABLE            *Xsdt,
  UINT32                Signature,
  DESCRIPTION_HEADER    **FoundTable
  )
{
  UINTN         Index;
  UINT32        EntryCount;
  UINT64        EntryPtr;
  UINTN         BasePtr;
  
  DESCRIPTION_HEADER    *Table;
  
  *FoundTable = NULL;
  
  EntryCount = (Xsdt->Header.Length - sizeof (DESCRIPTION_HEADER)) / sizeof(UINT64);
  
  BasePtr = (UINTN)(&(Xsdt->Entry));
  for (Index = 0; Index < EntryCount; Index ++) {
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * sizeof(UINT64)), sizeof(UINT64));
    Table = (DESCRIPTION_HEADER*)((UINTN)(EntryPtr));
    if (Table->Signature == Signature) {
      *FoundTable = Table;
      break;
    }
  }
  
  return;
}

VOID *
FindAcpiPtr (
  IN HOB_TEMPLATE  *Hob,
  UINT32           Signature
  )
{
  DESCRIPTION_HEADER    *AcpiTable;
  RSDP_TABLE            *Rsdp;
  RSDT_TABLE            *Rsdt;
  XSDT_TABLE            *Xsdt;
 
  AcpiTable = NULL;

  //
  // Check ACPI2.0 table
  //
  if ((int)Hob->Acpi20.Table != -1) {
    Rsdp = (RSDP_TABLE *)(UINTN)Hob->Acpi20.Table;
    Rsdt = (RSDT_TABLE *)(UINTN)Rsdp->RsdtAddress;
    Xsdt = NULL;
    if ((Rsdp->Revision >= 2) && (Rsdp->XsdtAddress < (UINT64)(UINTN)-1)) {
      Xsdt = (XSDT_TABLE *)(UINTN)Rsdp->XsdtAddress;
    }
    //
    // Check Xsdt
    //
    if (Xsdt != NULL) {
      ScanTableInXSDT (Xsdt, Signature, &AcpiTable);
    }
    //
    // Check Rsdt
    //
    if ((AcpiTable == NULL) && (Rsdt != NULL)) {
      ScanTableInRSDT (Rsdt, Signature, &AcpiTable);
    }
  }
  
  //
  // Check ACPI1.0 table
  //
  if ((AcpiTable == NULL) && ((int)Hob->Acpi.Table != -1)) {
    Rsdp = (RSDP_TABLE *)(UINTN)Hob->Acpi.Table;
    Rsdt = (RSDT_TABLE *)(UINTN)Rsdp->RsdtAddress;
    //
    // Check Rsdt
    //
    if (Rsdt != NULL) {
      ScanTableInRSDT (Rsdt, Signature, &AcpiTable);
    }
  }

  return AcpiTable;
}

#pragma pack(1)
//#define MCFG_SIGNATURE  0x4746434D
#define MCFG_SIGNATURE SIGNATURE_32 ('M', 'C', 'F', 'G')
typedef struct {
  UINT64  BaseAddress;
  UINT16  PciSegmentGroupNumber;
  UINT8   StartBusNumber;
  UINT8   EndBusNumber;
  UINT32  Reserved;
} MCFG_STRUCTURE;

#define FADT_SIGNATURE SIGNATURE_32 ('F', 'A', 'C', 'P')
typedef struct {
  DESCRIPTION_HEADER    Header;
  UINT32                FIRMWARE_CTRL;
  UINT32                DSDT;
  UINT8                 INT_MODEL;
  UINT8                 Preferred_PM_Profile;
  UINT16                SCI_INIT;
  UINT32                SMI_CMD;
  UINT8                 ACPI_ENABLE;
  UINT8                 ACPI_DISABLE;
  UINT8                 S4BIOS_REQ;
  UINT8                 PSTATE_CNT;
  UINT32                PM1a_EVT_BLK;
  UINT32                PM1b_EVT_BLK;
  UINT32                PM1a_CNT_BLK;
  UINT32                PM1b_CNT_BLK;
  UINT32                PM2_CNT_BLK;
  UINT32                PM_TMR_BLK;
  UINT32                GPE0_BLK;
  UINT32                GPE1_BLK;
  UINT8                 PM1_EVT_LEN;
  UINT8                 PM1_CNT_LEN;
  UINT8                 PM2_CNT_LEN;
  UINT8                 PM_TMR_LEN;
  UINT8                 GPE0_BLK_LEN;
  UINT8                 GPE1_BLK_LEN;
  UINT8                 GPE1_BASE;
  UINT8                 CST_CNT;
  UINT16                P_LVL2_LAT;
  UINT16                P_LVL3_LAT;
  UINT16                FLUSH_SIZE;
  UINT16                FLUSH_STRIDE;
  UINT8                 DUTY_OFFSET;
  UINT8                 DUTY_WIDTH;
  UINT8                 DAY_ALARM;
  UINT8                 MON_ALARM;
  UINT8                 CENTRY;
  UINT16                IAPC_BOOT_ARCH;
  UINT8                 Reserved_111;
  UINT32                Flags;
  GADDRESS_STRUCTURE    RESET_REG;
  UINT8                 RESET_VALUE;
  UINT8                 Reserved_129[3];
  UINT64                X_FIRMWARE_CTRL;
  UINT64                X_DSDT;
  GADDRESS_STRUCTURE    X_PM1a_EVT_BLK;
  GADDRESS_STRUCTURE    X_PM1b_EVT_BLK;
  GADDRESS_STRUCTURE    X_PM1a_CNT_BLK;
  GADDRESS_STRUCTURE    X_PM1b_CNT_BLK;
  GADDRESS_STRUCTURE    X_PM2_CNT_BLK;
  GADDRESS_STRUCTURE    X_PM_TMR_BLK;
  GADDRESS_STRUCTURE    X_GPE0_BLK;
  GADDRESS_STRUCTURE    X_GPE1_BLK;
} FADT_TABLE;

#pragma pack()

VOID
PrepareMcfgTable (
  IN HOB_TEMPLATE  *Hob
  )
{
  DESCRIPTION_HEADER    *McfgTable;
  MCFG_STRUCTURE        *Mcfg;
  UINTN                 McfgCount;
  UINTN                 Index;

  McfgTable = FindAcpiPtr (Hob, MCFG_SIGNATURE);
  if (McfgTable == NULL) {
    return ;
  }

  Mcfg = (MCFG_STRUCTURE *)((UINTN)McfgTable + sizeof(DESCRIPTION_HEADER) + sizeof(UINT64));
  McfgCount = (McfgTable->Length - sizeof(DESCRIPTION_HEADER) - sizeof(UINT64)) / sizeof(MCFG_STRUCTURE);

  //
  // Fill PciExpress info on Hob
  // Note: Only for 1st segment
  //
  for (Index = 0; Index < McfgCount; Index++) {
    if (Mcfg[Index].PciSegmentGroupNumber == 0) {
      Hob->PciExpress.PciExpressBaseAddressInfo.PciExpressBaseAddress = Mcfg[Index].BaseAddress;
      break;
    }
  }

  return ;
}

VOID
PrepareFadtTable (
  IN HOB_TEMPLATE  *Hob
  )
{
  FADT_TABLE            *Fadt;
  EFI_ACPI_DESCRIPTION  *AcpiDescription;

  Fadt = FindAcpiPtr (Hob, FADT_SIGNATURE);
  if (Fadt == NULL) {
    return ;
  }

  AcpiDescription = &Hob->AcpiInfo.AcpiDescription;
  //
  // Fill AcpiDescription according to FADT
  // Currently, only for PM_TMR
  //
  AcpiDescription->PM_TMR_LEN = Fadt->PM_TMR_LEN;
  AcpiDescription->TMR_VAL_EXT = (UINT8)((Fadt->Flags & 0x100) != 0);
  if ((Fadt->Header.Revision >= 3) && (Fadt->Header.Length >= sizeof(FADT_TABLE))) {
    CopyMem (
      &AcpiDescription->PM_TMR_BLK,
      &Fadt->X_PM_TMR_BLK,
      sizeof(GADDRESS_STRUCTURE)
      );
    CopyMem (
      &AcpiDescription->RESET_REG,
      &Fadt->RESET_REG,
      sizeof(GADDRESS_STRUCTURE)
      );
    AcpiDescription->RESET_VALUE = Fadt->RESET_VALUE;
  }
  if (AcpiDescription->PM_TMR_BLK.Address == 0) {
    AcpiDescription->PM_TMR_BLK.Address          = Fadt->PM_TMR_BLK;
    AcpiDescription->PM_TMR_BLK.AddressSpaceId   = ACPI_ADDRESS_ID_IO;
    AcpiDescription->PM_TMR_BLK.RegisterBitWidth = (UINT8) ((AcpiDescription->TMR_VAL_EXT == 0) ? 24 : 32);
  }

  return ;
}

VOID
PrepareHobLegacyTable (
  IN HOB_TEMPLATE  *Hob
  )
{
  CHAR8    PrintBuffer[256];

  Hob->Acpi.Table   = (EFI_PHYSICAL_ADDRESS)(UINTN)FindAcpiRsdPtr ();
  AsciiSPrint (PrintBuffer, 256, "\nAcpiTable=0x%x ", (UINT32)(UINTN)Hob->Acpi.Table);
  PrintString (PrintBuffer);
  Hob->Acpi20.Table = (EFI_PHYSICAL_ADDRESS)(UINTN)FindAcpiRsdPtr ();
  Hob->Smbios.Table = (EFI_PHYSICAL_ADDRESS)(UINTN)FindSMBIOSPtr ();
  AsciiSPrint (PrintBuffer, 256, "SMBIOS Table=0x%x ", (UINT32)(UINTN)Hob->Smbios.Table);
  PrintString (PrintBuffer);
  Hob->Mps.Table    = (EFI_PHYSICAL_ADDRESS)(UINTN)FindMPSPtr ();
  AsciiSPrint (PrintBuffer, 256, "MPS Table=0x%x\n", (UINT32)(UINTN)Hob->Mps.Table);
  PrintString (PrintBuffer);

  PrepareMcfgTable (Hob);

  PrepareFadtTable (Hob);

  return ;
}

