/** @file
  Helper Routines that use a PXE-enabled NIC option ROM.
 
Copyright (c) 1999 - 2017, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BiosSnp16.h"

#define TO_SEGMENT(x)   ((UINT16) (RShiftU64 ((UINT32)(UINTN) (x), 4) & 0xF000))
#define TO_OFFSET(x)    ((UINT16) ((UINT32)(UINTN) (x) & 0xFFFF))
#define PARAGRAPH_SIZE  0x10
#define IVT_BASE        0x00000000

#pragma pack(1)
typedef struct {
  UINT16  Signature;          ///< 0xaa55
  UINT8   ROMlength;          ///< size of this ROM in 512 byte blocks
  UINT8   InitEntryPoint[4];  ///< a jump to the initialization routine
  UINT8   Reserved[0xf];      ///< various
  UINT16  PxeRomIdOffset;     ///< offset of UNDI, $BC$, or BUSD ROM ID structure
  UINT16  PcirHeaderOffset;   ///< offset of PCI Expansion Header
  UINT16  PnpHeaderOffset;    ///< offset of Plug and Play Expansion Header
} OPTION_ROM_HEADER;
#pragma pack()

UINT32 CachedVectorAddress[0x100];

/**
 Cache Interrupt verctor address converted from IVT number.

 @param VectorNumber  IVT number
 
 @retval EFI_SUCCESS Success to operation.
**/
EFI_STATUS
CacheVectorAddress (
  UINT8   VectorNumber
  )
{
  UINT32  *Address;

  Address                          = (UINT32 *) ((UINTN) IVT_BASE + VectorNumber * 4);
  CachedVectorAddress[VectorNumber] = *Address;
  return EFI_SUCCESS;
}

/**
 Get interrupt vector address according to IVT number. 
  
 @param VectorNumber    Given IVT number
 
 @return cached interrupt vector address.
**/
EFI_STATUS
RestoreCachedVectorAddress (
  UINT8   VectorNumber
  )
{
  UINT32  *Address;

  Address  = (UINT32 *) ((UINTN) IVT_BASE + VectorNumber * 4);
  *Address = CachedVectorAddress[VectorNumber];
  return EFI_SUCCESS;
}

/**
 Print Undi loader table. 

 @param UndiLoaderStructure Point to Undi Loader table structure. 
 
**/
VOID
Print_Undi_Loader_Table (
  VOID *UndiLoaderStructure
  )
{
  UNDI_LOADER_T *DisplayPointer;

  DisplayPointer = (UNDI_LOADER_T *) UndiLoaderStructure;

  DEBUG ((DEBUG_NET, "Before Parsing the table contents, the table itself lives\n"));
  DEBUG ((DEBUG_NET, "\tat the address 0x%X\n\r", (UINT32)(UINTN) UndiLoaderStructure));

  DEBUG ((DEBUG_NET, "\n\rStatus = 0x%X\n\r", DisplayPointer->Status));
  DEBUG ((DEBUG_NET, "\t_AX_= 0x%X\n\r", DisplayPointer->Ax));
  DEBUG ((DEBUG_NET, "\t_BX_= 0x%X\n\r", DisplayPointer->Bx));
  DEBUG ((DEBUG_NET, "\t_DX_= 0x%X\n\r", DisplayPointer->Dx));
  DEBUG ((DEBUG_NET, "\t_DI_= 0x%X\n\r", DisplayPointer->Di));
  DEBUG ((DEBUG_NET, "\t_ES_= 0x%X\n\r", DisplayPointer->Es));
  DEBUG ((DEBUG_NET, "\tUNDI_DS= 0x%X\n\r", DisplayPointer->Undi_Ds));
  DEBUG ((DEBUG_NET, "\tUNDI_CS= 0x%X\n\r", DisplayPointer->Undi_Cs));
  DEBUG ((DEBUG_NET, "\tPXEptr:SEG= 0x%X\n\r", (UINT16) DisplayPointer->PXEptr.Segment));
  DEBUG ((DEBUG_NET, "\tPXEptr:OFF= 0x%X\n\r", (UINT16) DisplayPointer->PXEptr.Offset));
  DEBUG ((DEBUG_NET, "\tPXENVptr:SEG= 0x%X\n\r", (UINT16) DisplayPointer->PXENVptr.Segment));
  DEBUG ((DEBUG_NET, "\tPXENVptr:OFF= 0x%X\n\r", (UINT16) DisplayPointer->PXENVptr.Offset));
}

/**
  Simple table dumper.  The ROMID table is necessary in order to effect
  the "Early UNDI" trick.  Herein, the UNDI layer can be loaded in the
  pre-boot phase without having to download a Network Boot Program 
  across the wire.  It is required in the implementation in that we
  are not using PXE.

  @param RomIDStructure Point to RomID structure.
 
**/
VOID
Print_ROMID_Table (
  IN VOID *RomIDStructure
  )
{
  UNDI_ROMID_T  *DisplayPointer;

  DisplayPointer = (UNDI_ROMID_T *) RomIDStructure;

  DEBUG ((DEBUG_NET, "Before Parsing the table contents, the table itself lives\n"));
  DEBUG ((DEBUG_NET, "\tat the address 0x%X\n\r", (UINT32)(UINTN) RomIDStructure));

  DEBUG (
    (DEBUG_NET,
    "\n\rROMID %c%c%c%c\n\r",
    DisplayPointer->Signature[0],
    DisplayPointer->Signature[1],
    DisplayPointer->Signature[2],
    DisplayPointer->Signature[3])
    );

  DEBUG (
    (DEBUG_NET,
    "Length of this structure in bytes = 0x%X\n\r",
    DisplayPointer->StructLength)
    );
  DEBUG (
    (DEBUG_NET,
    "Use to make byte checksum of this structure == zero is = 0x%X\n\r",
    DisplayPointer->StructCksum)
    );
  DEBUG (
    (DEBUG_NET,
    "Structure format revision number= 0x%X\n\r",
    DisplayPointer->StructRev)
    );
  DEBUG (
    (DEBUG_NET,
    "API Revision number = 0x%X 0x%X 0x%X\n\r",
    DisplayPointer->UNDI_Rev[0],
    DisplayPointer->UNDI_Rev[1],
    DisplayPointer->UNDI_Rev[2])
    );
  DEBUG (
    (DEBUG_NET,
    "Offset of UNDI loader routine in the option ROM image= 0x%X\n\r",
    DisplayPointer->UNDI_Loader)
    );
  DEBUG ((DEBUG_NET, "From the data above, the absolute entry point of the UNDI loader is\n\r"));
  DEBUG (
    (DEBUG_NET,
    "\tat address 0x%X\n\r",
    (UINT32) (DisplayPointer->UNDI_Loader + ((UINT32) (UINTN)(DisplayPointer - 0x20) & 0xFFFF0)))
    );
  DEBUG ((DEBUG_NET, "Minimum stack segment size, in bytes,\n\r"));
  DEBUG (
    (DEBUG_NET,
    "needed to load and run the UNDI= 0x%X \n\r",
    DisplayPointer->StackSize)
    );
  DEBUG (
    (DEBUG_NET,
    "UNDI runtime code and data = 0x%X\n\r",
    DisplayPointer->DataSize)
    );
  DEBUG (
    (DEBUG_NET,
    "Segment size = 0x%X\n\r",
    DisplayPointer->CodeSize)
    );
  DEBUG (
    (DEBUG_NET,
    "\n\rBus Type =  %c%c%c%c\n\r",
    DisplayPointer->BusType[0],
    DisplayPointer->BusType[1],
    DisplayPointer->BusType[2],
    DisplayPointer->BusType[3])
    );
}

/**
  Print PXE table.

  @param PxeTable Point to PXE table structure
  
**/
VOID
Print_PXE_Table (
  IN VOID*  PxeTable
  )
{
  PXE_T *DisplayPointer;
  UINTN Index;
  UINT8 *Dptr;

  DisplayPointer  = (PXE_T *) PxeTable;
  Dptr            = (UINT8 *) PxeTable;

  DEBUG ((DEBUG_NET, "This is the PXE table at address 0x%X\n\r", PxeTable));

  DEBUG ((DEBUG_NET, "A dump of the 0x%X bytes is:\n\r", sizeof (PXE_T)));

  for (Index = 0; Index < sizeof (PXE_T); Index++) {
    if ((Index % 0x10) == 0) {
      DEBUG ((DEBUG_NET, "\t\n\r"));
    }

    DEBUG ((DEBUG_NET, " 0x%X  ", *Dptr++));
  }

  DEBUG ((DEBUG_NET, "\n\r"));
  DEBUG (
    (DEBUG_NET,
    "\n\rPXE %c%c%c%c%c%c\n\r",
    DisplayPointer->Signature[0],
    DisplayPointer->Signature[1],
    DisplayPointer->Signature[2],
    DisplayPointer->Signature[3])
    );
  DEBUG (
    (DEBUG_NET,
    "Length of this structure in bytes = 0x%X\n\r",
    DisplayPointer->StructLength)
    );
  DEBUG (
    (DEBUG_NET,
    "Use to make byte checksum of this  structure == zero is = 0x%X\n\r",
    DisplayPointer->StructCksum)
    );
  DEBUG (
    (DEBUG_NET,
    "Structure format revision number = 0x%X\n\r",
    DisplayPointer->StructRev)
    );
  DEBUG (
    (DEBUG_NET,
    "Must be zero, is equal to 0x%X\n\r",
    DisplayPointer->Reserved1)
    );
  DEBUG (
    (DEBUG_NET,
    "Far pointer to UNDI ROMID = 0x%X\n\r",
    (UINT32) (DisplayPointer->Undi.Segment << 0x4 | DisplayPointer->Undi.Offset))
    );
  DEBUG (
    (DEBUG_NET,
    "Far pointer to base-code ROMID = 0x%X\n\r",
    (UINT32) ((DisplayPointer->Base.Segment << 0x04) | DisplayPointer->Base.Offset))
    );
  DEBUG ((DEBUG_NET, "16bit stack segment API entry point.  This will be seg:off in \n\r"));
  DEBUG (
    (DEBUG_NET,
    "real mode and sel:off in 16:16 protected mode = 0x%X:0x%X\n\r",
    DisplayPointer->EntryPointSP.Segment,
    DisplayPointer->EntryPointSP.Offset)
    );

  DEBUG ((DEBUG_NET, "\n\tNOTE to the implementer\n\tThis is the entry to use for call-ins\n\r"));

  DEBUG ((DEBUG_NET, "32bit stack Segment API entry point.  This will be sel:off. \n\r"));
  DEBUG (
    (DEBUG_NET,
    "In real mode, sel == 0 = 0x%X:0x%X\n\r",
    DisplayPointer->EntryPointESP.Segment,
    DisplayPointer->EntryPointESP.Offset)
    );
  DEBUG (
    (DEBUG_NET,
    "Reserved2 value, must be zero, is equal to 0x%X\n\r",
    DisplayPointer->Reserved2)
    );
  DEBUG (
    (DEBUG_NET,
    "Number of segment descriptors in this structur = 0x%X\n\r",
    (UINT8) DisplayPointer->SegDescCnt)
    );
  DEBUG (
    (DEBUG_NET,
    "First segment descriptor in GDT assigned to PXE = 0x%X\n\r",
    (UINT16) DisplayPointer->FirstSelector)
    );
  DEBUG (
    (DEBUG_NET,
    "The Stack is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->Stack.Seg_Addr,
    (UINT32) DisplayPointer->Stack.Phy_Addr,
    (UINT16) DisplayPointer->Stack.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The UNDIData is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->UNDIData.Seg_Addr,
    (UINT32) DisplayPointer->UNDIData.Phy_Addr,
    (UINT16) DisplayPointer->UNDIData.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The UNDICodeWrite is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->UNDICode.Seg_Addr,
    (UINT32) DisplayPointer->UNDICode.Phy_Addr,
    (UINT16) DisplayPointer->UNDICode.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The Stack is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->UNDICodeWrite.Seg_Addr,
    (UINT32) DisplayPointer->UNDICodeWrite.Phy_Addr,
    (UINT16) DisplayPointer->UNDICodeWrite.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The BC_Data is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->BC_Data.Seg_Addr,
    (UINT32) DisplayPointer->BC_Data.Phy_Addr,
    (UINT16) DisplayPointer->BC_Data.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The BC_Code is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->BC_Code.Seg_Addr,
    (UINT32) DisplayPointer->BC_Code.Phy_Addr,
    (UINT16) DisplayPointer->BC_Code.Seg_Size)
    );
  DEBUG (
    (DEBUG_NET,
    "The BC_CodeWrite is \n\r\tSegment Addr = 0x%X\n\r\tPhysical Addr = 0x%X\n\r\tSeg Size = 0x%X\n\r",
    (UINT16) DisplayPointer->BC_CodeWrite.Seg_Addr,
    (UINT32) DisplayPointer->BC_CodeWrite.Phy_Addr,
    (UINT16) DisplayPointer->BC_CodeWrite.Seg_Size)
    );
}

/**
  Print PXENV table.

  @param PxenvTable Point to PXENV
  
**/
VOID
Print_PXENV_Table (
  IN VOID *PxenvTable
  )
{
  PXENV_T *DisplayPointer;

  DisplayPointer = (PXENV_T *) PxenvTable;

  DEBUG (
    (DEBUG_NET,
    "\n\rPXENV+ %c%c%c%c%c%c\n\r",
    DisplayPointer->Signature[0],
    DisplayPointer->Signature[1],
    DisplayPointer->Signature[2],
    DisplayPointer->Signature[3],
    DisplayPointer->Signature[4],
    DisplayPointer->Signature[5])
    );

  DEBUG (
    (DEBUG_NET,
    "PXE version number.  \n\r\tLSB is minor version.  \n\r\tMSB is major version = 0x%X\n\r",
    DisplayPointer->Version)
    );
  DEBUG (
    (DEBUG_NET,
    "Length of PXE-2.0 Entry Point structure in bytes = 0x%X\n\r",
    DisplayPointer->StructLength)
    );
  DEBUG ((DEBUG_NET, "Used to make structure checksum equal zero is now = 0x%X\n\r", DisplayPointer->StructCksum));
  DEBUG ((DEBUG_NET, "Real mode API entry point  segment:Offset.  = 0x%X\n\r", DisplayPointer->RMEntry));
  DEBUG ((DEBUG_NET, "Protected mode API entry point = 0x%X\n\r", DisplayPointer->PMEntryOff));
  DEBUG ((DEBUG_NET, " segment:Offset.  This will always be zero.  \n\r"));
  DEBUG ((DEBUG_NET, "Protected mode API calls = 0x%X\n\r", DisplayPointer->PMEntrySeg));
  DEBUG ((DEBUG_NET, "Real mode stack segment = 0x%X\n\r", DisplayPointer->StackSeg));
  DEBUG ((DEBUG_NET, "Stack segment size in bytes = 0x%X\n\r", DisplayPointer->StackSize));
  DEBUG ((DEBUG_NET, "Real mode base-code code segment = 0x%X\n\r", DisplayPointer->BaseCodeSeg));
  DEBUG ((DEBUG_NET, "Base-code code segment size = 0x%X\n\r", DisplayPointer->BaseCodeSize));
  DEBUG ((DEBUG_NET, "Real mode base-code data segment = 0x%X\n\r", DisplayPointer->BaseDataSeg));
  DEBUG ((DEBUG_NET, "Base-code data segment size = 0x%X\n\r", DisplayPointer->BaseDataSize));

  DEBUG (
    (DEBUG_NET,
    "UNDI code segment size in bytes = 0x%X\n\r",
    DisplayPointer->UNDICodeSize)
    );
  DEBUG (
    (DEBUG_NET,
    "Real mode segment:Offset pointer \n\r\tto PXE Runtime ID structure, address = 0x%X\n\r",
    DisplayPointer->RuntimePtr)
    );
  DEBUG (
    (
    DEBUG_NET,
    "From above, we have a linear address of 0x%X\n\r",
    (UINT32)
    (
    ((UINT32)(UINTN)(DisplayPointer->RuntimePtr) & 0xFFFF) +
    (((UINT32)(UINTN)(DisplayPointer->RuntimePtr) & 0xFFFF0000) >> 12)
    )
    )
    );
}


#define OPTION_ROM_PTR  ((OPTION_ROM_HEADER *) RomAddress)

/**
  If available, launch the BaseCode from a NIC option ROM.
  This should install the !PXE and PXENV+ structures in memory for
  subsequent use.
  

  @param SimpleNetworkDevice    Simple network device instance
  @param RomAddress             The ROM base address for NIC rom.
  
  @retval EFI_NOT_FOUND         The check sum does not match 
  @retval EFI_NOT_FOUND         Rom ID offset is wrong 
  @retval EFI_NOT_FOUND         No Rom ID structure is found 
**/
EFI_STATUS
LaunchBaseCode (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  UINTN                   RomAddress
  )
{
  EFI_STATUS            Status;
  EFI_IA32_REGISTER_SET InOutRegs;
  UNDI_ROMID_T          *RomIdTableAddress;
  UNDI_LOADER_T         *UndiLoaderTable;
  UINT16                Segment;
  UINT16                *StackPointer;
  VOID                  *Buffer;
  UINTN                 Size;
  PXE_T                 *Pxe;
  UINT32                RomLength;
  UINTN                 PciSegment;
  UINTN                 Bus;
  UINTN                 Device;
  UINTN                 Function;
  BOOLEAN               ThunkFailed;

  DEBUG ((DEBUG_NET, "\n\r\n\rCheck for the UNDI ROMID Signature\n\r"));

  //
  // paranoia - check structures for validity
  //
  RomLength = OPTION_ROM_PTR->ROMlength << 9;
  if (CalculateSum8 ((UINT8 *) RomAddress, RomLength) != 0) {
    DEBUG ((DEBUG_ERROR, "ROM Header Checksum Error\n\r"));
    return EFI_NOT_FOUND;
  }

  RomIdTableAddress = (UNDI_ROMID_T *) (RomAddress + OPTION_ROM_PTR->PxeRomIdOffset);

  if (((UINT32)OPTION_ROM_PTR->PxeRomIdOffset + RomIdTableAddress->StructLength) > RomLength) {
    DEBUG ((DEBUG_ERROR, "ROM ID Offset Error\n\r"));
    return EFI_NOT_FOUND;
  }
  //
  // see if this is a header for an UNDI ROM ID structure (vs. a $BC$ or BUSD type)
  //
  if (CompareMem (RomIdTableAddress->Signature, UNDI_ROMID_SIG, sizeof RomIdTableAddress->Signature) != 0) {
    DEBUG ((DEBUG_ERROR, "No ROM ID Structure found....\n\r"));
    return EFI_NOT_FOUND;
    //
    // its not - keep looking
    //
  }

  if (CalculateSum8 ((UINT8 *) RomIdTableAddress, RomIdTableAddress->StructLength) != 0) {
    DEBUG ((DEBUG_ERROR, "ROM ID Checksum Error\n\r"));
    return EFI_NOT_FOUND;
  }

  Print_ROMID_Table (RomIdTableAddress);

  DEBUG (
    (DEBUG_NET,
    "The ROM ID is located at 0x%X\n\r",
    RomIdTableAddress)
    );

  DEBUG (
    (DEBUG_NET,
    "With an UNDI Loader located at 0x%X\n\r",
    RomAddress + RomIdTableAddress->UNDI_Loader)
    );

  //
  // found an UNDI ROM ID structure
  //
  SimpleNetworkDevice->Nii.ImageAddr  = RomAddress;
  SimpleNetworkDevice->Nii.ImageSize  = RomLength;
  SimpleNetworkDevice->Nii.MajorVer   = RomIdTableAddress->UNDI_Rev[2];
  SimpleNetworkDevice->Nii.MinorVer   = RomIdTableAddress->UNDI_Rev[1];

  DEBUG ((DEBUG_NET, "Allocate area for the UNDI_LOADER_T structure\n\r"));
  //
  // Allocate 1 page below 1MB to put real mode thunk code in
  //
  // Undi Loader Table is a PXE Specification prescribed data structure
  // that is used to transfer information into and out of the Undi layer.
  // Note how it must be located below 1 MB.
  //
  SimpleNetworkDevice->UndiLoaderTablePages = EFI_SIZE_TO_PAGES (PARAGRAPH_SIZE + sizeof (UNDI_LOADER_T));
  Status = BiosSnp16AllocatePagesBelowOneMb (
            SimpleNetworkDevice->UndiLoaderTablePages,
            &SimpleNetworkDevice->UndiLoaderTable
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "We had a failure in AllocatePages, status code = 0x%X\n", Status));
    return EFI_OUT_OF_RESOURCES;
  }

  UndiLoaderTable = SimpleNetworkDevice->UndiLoaderTable;

  DEBUG ((DEBUG_NET, "Allocate area for the real-mode stack whose sole purpose\n\r"));
  DEBUG ((DEBUG_NET, "in life right now is to store a SEG:OFFSET combo pair that\n\r"));
  DEBUG ((DEBUG_NET, "points to an Undi_Loader_t table structure\n\r"));

  Size    = 0x100;
  Status  = gBS->AllocatePool (EfiLoaderData, Size, &Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Now we want to put a pointer to the Under Loader Table in our MemPage
  // Buffer.  This will be the argument stack for the call into the Undi Loader
  //
  StackPointer    = (UINT16 *) Buffer;
  *StackPointer++ = TO_OFFSET (UndiLoaderTable);
  //
  // push the OFFSET
  //
  *StackPointer++ = TO_SEGMENT (UndiLoaderTable);
  //
  // push the SEGMENT
  //
  StackPointer = (UINT16 *) Buffer;
  //
  // reset the stack pointer
  //
  DEBUG (
    (DEBUG_NET,
    "After the fixups, the stack pointer is 0x%X\n\r",
    (UINT64)(UINTN) StackPointer)
    );

  //
  // Allocate memory for the Deployed UNDI.
  // The UNDI is essentially telling us how much space it needs, and
  // it is up to the EFI driver to allocate sufficient, boot-time
  // persistent resources for the call
  //
  SimpleNetworkDevice->DestinationDataSegmentPages = EFI_SIZE_TO_PAGES (RomIdTableAddress->DataSize);
  Status = BiosSnp16AllocatePagesBelowOneMb (
            SimpleNetworkDevice->DestinationDataSegmentPages,
            &SimpleNetworkDevice->DestinationDataSegment
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "We had a failure in AllocatePages, status code = 0x%X\n", Status));
    return Status;
  }

  UndiLoaderTable->Undi_Ds = (UINT16) ((UINTN) SimpleNetworkDevice->DestinationDataSegment >> 4);

  //
  // Allocate memory for the Deployed UNDI stack
  // The UNDI is essentially telling us how much space it needs, and
  // it is up to the EFI driver to allocate sufficient, boot-time
  // persistent resources for the call
  //
  SimpleNetworkDevice->DestinationStackSegmentPages = EFI_SIZE_TO_PAGES (RomIdTableAddress->StackSize);
  Status = BiosSnp16AllocatePagesBelowOneMb (
            SimpleNetworkDevice->DestinationStackSegmentPages,
            &SimpleNetworkDevice->DestinationStackSegment
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "We had a failure in AllocatePages, status code = 0x%X\n", Status));
    return Status;
  }
  //
  // Allocate memory for the Deployed UNDI.
  // The UNDI is essentially telling us how much space it needs, and
  // it is up to the EFI driver to allocate sufficient, boot-time
  // persistent resources for the call
  //
  SimpleNetworkDevice->DestinationCodeSegmentPages = EFI_SIZE_TO_PAGES (RomIdTableAddress->CodeSize);
  Status = BiosSnp16AllocatePagesBelowOneMb (
            SimpleNetworkDevice->DestinationCodeSegmentPages,
            &SimpleNetworkDevice->DestinationCodeSegment
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "We had a failure in AllocatePages, status code = 0x%X\n", Status));
    return Status;
  }

  UndiLoaderTable->Undi_Cs = (UINT16) ((UINTN) SimpleNetworkDevice->DestinationCodeSegment >> 4);

  //
  // these are in the Input and Output Parameter to be sent to the UNDI Loader code
  //
  UndiLoaderTable->Status = 0xAA55;
  //
  // -------------------- Changed by Michael_Huang@3Com.com -----------------
  // UndiLoaderTable->_AX is AX value when UNDI ROM is initialized by BIOS, it is the PCI bus device
  // function of the NIC. Please refer to PXE Spec for detail info.
  // old code is:
  // UndiLoaderTable->Ax       = 0x0;
  // -----------------------------------------------------------------------
  //
  SimpleNetworkDevice->PciIo->GetLocation (
                                SimpleNetworkDevice->PciIo,
                                &PciSegment,
                                &Bus,
                                &Device,
                                &Function
                                );
  UndiLoaderTable->Ax = (UINT16) ((Bus << 0x8) | (Device << 0x3) | (Function));
  UndiLoaderTable->Bx = 0x0;
  UndiLoaderTable->Dx = 0x0;
  UndiLoaderTable->Di = 0x0;
  UndiLoaderTable->Es = 0x0;

  //
  // set these OUT values to zero in order to ensure that
  // uninitialized memory is not mistaken for display data
  //
  UndiLoaderTable->PXEptr.Offset    = 0;
  UndiLoaderTable->PXEptr.Segment   = 0;
  UndiLoaderTable->PXENVptr.Segment = 0;
  UndiLoaderTable->PXENVptr.Offset  = 0;

  DEBUG (
    (DEBUG_INIT,
    "The NIC is located at Bus 0x%X, Device 0x%X, Function 0x%X\n\r",
    Bus,
    Device,
    Function)
    );

  //
  // These are the values that set up the ACTUAL IA32 machine state, whether in
  // Real16 in EFI32 or the IVE for IA64
  // register values are unused except for CS:IP and SS:SP
  //
  InOutRegs.X.AX  = 0;
  InOutRegs.X.BX  = 0;
  InOutRegs.X.CX  = 0;
  InOutRegs.X.DX  = 0;
  InOutRegs.X.SI  = 0;
  InOutRegs.X.DI  = 0;
  InOutRegs.X.BP  = 0;
  InOutRegs.X.DS  = 0;
  InOutRegs.X.ES  = 0;
  //
  // just to be clean
  //
  DEBUG ((DEBUG_NET, "The way this game works is that the SS:SP +4 should point\n\r"));
  DEBUG ((DEBUG_NET, "to the contents of the UndiLoaderTable\n\r"));
  DEBUG (
    (DEBUG_NET,
    "The Undi Loader Table is at address = 0x%X\n\r",
    (UINT32)(UINTN) UndiLoaderTable)
    );
  DEBUG (
    (DEBUG_NET,
    "The segment and offsets are 0x%X and 0x%X, resp\n",
    TO_SEGMENT (UndiLoaderTable),
    TO_OFFSET (UndiLoaderTable))
    );

  DEBUG (
    (DEBUG_NET,
    "The Linear Address of the UNDI Loader entry is 0x%X\n",
    RomAddress + RomIdTableAddress->UNDI_Loader)
    );

  DEBUG (
    (DEBUG_NET,
    "The Address offset of the UNDI Loader entry is 0x%X\n",
    RomIdTableAddress->UNDI_Loader)
    );

  DEBUG ((DEBUG_NET, "Before the call, we have...\n\r"));
  Print_Undi_Loader_Table (UndiLoaderTable);

  Segment = ((UINT16) (RShiftU64 (RomAddress, 4) & 0xFFFF));
  DEBUG ((DEBUG_NET, "The Segment of the call is 0x%X\n\r", Segment));

  //
  // make the call into the UNDI Code
  //
  DEBUG ((DEBUG_INIT, "Make the call into the UNDI code now\n\r"));

  DEBUG ((DEBUG_NET, "\nThe 20-BIt address of the Call, and the location \n\r"));
  DEBUG ((DEBUG_NET, "\twhere we should be able to set a breakpoint is \n\r"));
  DEBUG (
    (DEBUG_NET,
    "\t\t0x%X, from SEG:OFF 0x%X:0x%X\n\r\n\r",
    Segment * 0x10 + RomIdTableAddress->UNDI_Loader,
    Segment,
    RomIdTableAddress->UNDI_Loader)
    );

  ThunkFailed = SimpleNetworkDevice->LegacyBios->FarCall86 (
                                                   SimpleNetworkDevice->LegacyBios,
                                                   Segment,  // Input segment
                                                   (UINT16) RomIdTableAddress->UNDI_Loader,  // Offset
                                                   &InOutRegs,                               // Ptr to Regs
                                                   Buffer,                                   // Reference to Stack
                                                   Size                                      // Size of the Stack
                                                   );
  if (ThunkFailed) {
    return EFI_ABORTED;
  }

  DEBUG (
    (DEBUG_NET,
    "The return code UndiLoaderTable->Status is = 0x%X\n\r",
    UndiLoaderTable->Status)
    );
  DEBUG (
    (DEBUG_NET,
    "This error code should match eax, which is = 0x%X\n\r",
    InOutRegs.X.AX)
    );

  if ((UndiLoaderTable->Status != 0) || (InOutRegs.X.AX != PXENV_EXIT_SUCCESS)) {
    DEBUG ((DEBUG_NET, "LaunchBaseCode exits with error, RomAddress = 0x%X\n\r", RomAddress));
    return EFI_ABORTED;
  }

  DEBUG ((DEBUG_NET, "Now returned from the UNDI code\n\r"));

  DEBUG ((DEBUG_NET, "After the call, we have...\n\r"));
  Print_Undi_Loader_Table (UndiLoaderTable);

  DEBUG ((DEBUG_NET, "Display the PXENV+ and !PXE tables exported by NIC\n\r"));
  Print_PXENV_Table ((VOID *)(((UINTN)UndiLoaderTable->PXENVptr.Segment << 4) | UndiLoaderTable->PXENVptr.Offset));
  Print_PXE_Table ((VOID *)(((UINTN)UndiLoaderTable->PXEptr.Segment << 4) + UndiLoaderTable->PXEptr.Offset));

  Pxe = (PXE_T *)(((UINTN)UndiLoaderTable->PXEptr.Segment << 4) + UndiLoaderTable->PXEptr.Offset);
  SimpleNetworkDevice->Nii.Id = (UINT64)(UINTN) Pxe;

  gBS->FreePool (Buffer);

  //
  // paranoia - make sure a valid !PXE structure
  //
  if (CompareMem (Pxe->Signature, PXE_SIG, sizeof Pxe->Signature) != 0) {
    DEBUG ((DEBUG_ERROR, "!PXE Structure not found....\n\r"));
    return EFI_NOT_FOUND;
    //
    // its not - keep looking
    //
  }

  if (CalculateSum8 ((UINT8 *) Pxe, Pxe->StructLength) != 0) {
    DEBUG ((DEBUG_ERROR, "!PXE Checksum Error\n\r"));
    return EFI_NOT_FOUND;
  }

  if (Pxe->StructLength < (UINT8 *) &Pxe->FirstSelector - (UINT8 *) Pxe->Signature) {
    DEBUG ((DEBUG_ERROR, "!PXE Length Error\n\r"));
    return EFI_NOT_FOUND;
  }

  if ((((UINTN) Pxe->Undi.Segment) << 4) + Pxe->Undi.Offset != (UINTN) RomIdTableAddress) {
    DEBUG ((DEBUG_ERROR, "!PXE RomId Address Error\n\r"));
    return EFI_NOT_FOUND;
  }
  //
  // This is the magic to bind the global PXE interface
  // This dirtiness is for non-protocol shrouded access
  //
  SimpleNetworkDevice->PxeEntrySegment = Pxe->EntryPointSP.Segment;

  if (SimpleNetworkDevice->PxeEntrySegment == 0) {
    DEBUG ((DEBUG_ERROR, "!PXE EntryPointSP segment Error\n\r"));
    return EFI_NOT_FOUND;
  }

  SimpleNetworkDevice->PxeEntryOffset = Pxe->EntryPointSP.Offset;

  DEBUG (
    (
    DEBUG_NET, "The entry point is 0x%X:0x%X\n\r", SimpleNetworkDevice->PxeEntrySegment, SimpleNetworkDevice->
    PxeEntryOffset
    )
    );

  return EFI_SUCCESS;
}

/**
  Effect the Far Call into the PXE Layer

  Note: When using a 32-bit stack segment do not push 32-bit words onto the stack. The PXE API
  services will not work, unless there are three 16-bit parameters pushed onto the stack.
      push DS                                 ;Far pointer to parameter structure
      push offset pxe_data_call_struct        ;is pushed onto stack.
      push Index                              ;UINT16 is pushed onto stack.
      call dword ptr (s_PXE ptr es:[di]).EntryPointSP
      add sp, 6 ;Caller cleans up stack.  

  @param SimpleNetworkDevice    Device instance for simple network
  @param Table                 Point to parameter/retun value table for legacy far call
  @param TableSize              The size of parameter/return value table
  @param CallIndex              The index of legacy call.
  
  @return EFI_STATUS 
**/
EFI_STATUS
MakePxeCall (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT VOID             *Table,
  IN UINTN                TableSize,
  IN UINT16               CallIndex
  )
{
  EFI_STATUS            Status;
  EFI_IA32_REGISTER_SET InOutRegs;
  UINT16                *BPtr;
  VOID                  *Buffer;
  UINTN                 Size;
  VOID                  *MemPageAddress;
  UINTN                 Index;
  BOOLEAN               ThunkFailed;

  DEBUG ((DEBUG_NET, "MakePxeCall(CallIndex = %02x, Table = %X, TableSize = %d)\n", CallIndex, Table, TableSize));

  if (SimpleNetworkDevice->PxeEntrySegment == 0 && SimpleNetworkDevice->PxeEntryOffset == 0) {
    return EFI_DEVICE_ERROR;
  }

  Status = EFI_SUCCESS;

  //
  // Allocate a transient data structure for the argument table
  // This table needs to have the input XXX_t structure copied into here.
  // The PXE UNDI can only grab this table when it's below one-MB, and
  // this implementation will not try to push this table on the stack
  // (although this is a possible optimization path since EFI always allocates
  // 4K as a minimum page size...............)
  //
  Status = BiosSnp16AllocatePagesBelowOneMb (
            TableSize / EFI_PAGE_SIZE + 1,
            &MemPageAddress
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "We had a failure in AllocatePages, status code = 0x%X\n", Status));
    return Status;
  }
  //
  // Copy the > 1MB pool table to a sub-1MB buffer
  //
  CopyMem (MemPageAddress, Table, TableSize);

  //
  // Allocate space for IA-32 register context
  //
  ZeroMem (&InOutRegs, sizeof (InOutRegs));
  InOutRegs.X.ES  = SimpleNetworkDevice->PxeEntrySegment;
  InOutRegs.X.DI  = SimpleNetworkDevice->PxeEntryOffset;

  //
  // The game here is to build the stack which will subsequently
  // get copied down below 1 MB by the FarCall primitive.
  // This is now our working stack
  //
  Size = 6;
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  Size,
                  &Buffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BPtr    = (UINT16 *) Buffer;
  *BPtr++ = CallIndex;
  //
  // SP + 2
  //
  *BPtr++ = TO_OFFSET (MemPageAddress);
  *BPtr++ = TO_SEGMENT (MemPageAddress);

  DEBUG ((DEBUG_NET, "State before FarCall86\n"));
  DEBUG ((DEBUG_NET, "The Buffer is at 0x%X\n\r", Buffer));
  BPtr = (UINT16 *) Buffer;
  DEBUG ((DEBUG_NET, "  Buffer  = %04X %04X %04X", *BPtr, *(BPtr + 1), *(BPtr + 2)));
  DEBUG ((DEBUG_NET, "  MemPage = "));
  for (Index = 0; Index < TableSize; Index++) {
    DEBUG ((DEBUG_NET, " %02x", *((UINT8 *) MemPageAddress + Index)));
  }

  DEBUG ((DEBUG_NET, "\n"));

  ThunkFailed = SimpleNetworkDevice->LegacyBios->FarCall86 (
                                                   SimpleNetworkDevice->LegacyBios,
                                                   SimpleNetworkDevice->PxeEntrySegment, // Input segment
                                                   SimpleNetworkDevice->PxeEntryOffset,
                                                   &InOutRegs,                           // Ptr to Regs
                                                   Buffer,                               // Reference to Stack
                                                   6                                     // Size of the Stack
                                                   );
  if (ThunkFailed) {
    return EFI_ABORTED;
  }

  DEBUG ((DEBUG_NET, "State after FarCall86\n"));
  DEBUG ((DEBUG_NET, "The Buffer is at 0x%X\n\r", Buffer));
  BPtr = (UINT16 *) Buffer;
  DEBUG ((DEBUG_NET, "  Buffer  = %04X %04X %04X", *BPtr, *(BPtr + 1), *(BPtr + 2)));
  DEBUG ((DEBUG_NET, "  MemPage = "));
  for (Index = 0; Index < TableSize; Index++) {
    DEBUG ((DEBUG_NET, " %02x", *((UINT8 *) MemPageAddress + Index)));
  }

  DEBUG ((DEBUG_NET, "\n"));

  //
  // Copy the sub 1MB table to > 1MB table
  //
  CopyMem (Table, MemPageAddress, TableSize);

  //
  // For PXE UNDI call, AX contains the return status.
  // Convert the PXE UNDI Status to EFI_STATUS type
  //
  if (InOutRegs.X.AX == PXENV_EXIT_SUCCESS) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_DEVICE_ERROR;
  }
  //
  // Clean up house
  //
  gBS->FreePool (Buffer);
  gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) MemPageAddress, TableSize / EFI_PAGE_SIZE + 1);

  return Status;
}
