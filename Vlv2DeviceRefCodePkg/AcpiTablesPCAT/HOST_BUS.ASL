/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  HOST_BUS.ASL

Abstract:

  Baytrail PCI configuration space definition.

---*/
Device(VLVC)
{
  Name(_ADR, 0x00000000)           // Device 0, Function 0

  // Define various MCH Controller PCI Configuration Space
  // registers which will be used to dynamically produce all
  // resources in the Host Bus _CRS.
  OperationRegion(HBUS, PCI_Config, 0x00, 0xFF)
  Field(HBUS, DWordAcc, NoLock, Preserve)
  {
    Offset(0xD0),
    SMCR,   32,             // VLV Message Control Register (0xD0)
    Offset(0xD4),
    SMDR,   32,             // VLV Message Data Register (0xD4)
    Offset(0xD8),
    MCRX,   32,             // VLV Message Control Register Extension (0xD8)
  }

  // Define a method to read a 32-bit register on the VLV Message bus.
  //  Arg0 = Port
  //  Arg1 = Register
  //
  //  Returns 32-bit register value

  Method(RMBR, 2, Serialized)
  {

    // Initiate regsiter read message on VLV Message Bus MCR

    Or(ShiftLeft(Arg0, 16), ShiftLeft(Arg1, 8), Local0)
    Or(0x100000F0, Local0, SMCR)

    // Read register value from Message Data Register

    Return(SMDR)
  }


  // Define a method to write a 32-bit register on the VLV Message bus MDR.
  //  Arg0 = Port
  //  Arg1 = Register
  //  Arg2 = 32-bit value

  Method(WMBR, 3, Serialized)
  {

    // Write register value to Message Data Register

    Store(Arg2, SMDR)

    // Initiate register write message on VLV Message Bus

    Or(ShiftLeft(Arg0, 16), ShiftLeft(Arg1, 8), Local0)
    Or(0x110000F0, Local0, SMCR)
  }
}

//
// BUS, I/O, and MMIO resources
//
Method(_CRS,0,Serialized)
{
  //Update ISP0 reserved memory
  CreateDwordField(RES0, ^ISP0._MIN,ISMN)
  CreateDwordField(RES0, ^ISP0._MAX,ISMX)
  CreateDwordField(RES0, ^ISP0._LEN,ISLN)
  If (LEqual(ISPD,1))
  {
    Store (ISPA, ISMN)
    Add (ISMN, ISLN, ISMX)
    Subtract(ISMX, 1, ISMX)
  } Else
  {
    Store (0, ISMN)
    Store (0, ISMX)
    Store (0, ISLN)
  }

  //PCI MMIO SPACE
  CreateDwordField(RES0, ^PM01._MIN,M1MN)
  CreateDwordField(RES0, ^PM01._MAX,M1MX)
  CreateDwordField(RES0, ^PM01._LEN,M1LN)

  //Get dBMBOUND Base
  And(BMBD, 0xFF000000, M1MN)

  //Get ECBASE
  Store(PCIT, M1MX)
  Add(Subtract(M1MX, M1MN), 1, M1LN)
  Subtract(M1MX, 1, M1MX)

  // Create pointers to Gfx Stolen Memory Sizing values.
  CreateDwordField(RES0, ^STOM._MIN,GSMN)
  CreateDwordField(RES0, ^STOM._MAX,GSMX)
  CreateDwordField(RES0, ^STOM._LEN,GSLN)

  If (LNotEqual (\_SB.PCI0.GFX0.GSTM, 0xFFFFFFFF))
  {
    Store(0x00, GSMN) //Read the Stolen memory base from B0:D2:F0:R5C
  } else
  {
    Store(\_SB.PCI0.GFX0.GSTM, GSMN) //Read the Stolen memory base from B0:D2:F0:R5C
  }
  If (LNotEqual (\_SB.PCI0.GFX0.GUMA, 0xFFFFFFFF))
  {
    Store(0x00, GSLN) //Read the Stolen memory base from B0:D2:F0:R5C
  } else
  {
    ShiftLeft(\_SB.PCI0.GFX0.GUMA, 25, GSLN) //Read Stolen memory base form B0:D2:F0:R50
  }
  Add(GSMN, GSLN, GSMX) //Store the Stolen Memory Size
  Subtract(GSMX, 1, GSMX)

  Return(RES0)
}

Name( RES0,ResourceTemplate()
{
  WORDBusNumber (          // Bus number resource (0); the bridge produces bus numbers for its subsequent buses
    ResourceProducer,      // bit 0 of general flags is 1
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,             // PosDecode
    0x0000,                // Granularity
    0x0000,                // Min
    0x00FF,                // Max
    0x0000,                // Translation
    0x0100                 // Range Length = Max-Min+1
  )

  IO (Decode16, 0x70, 0x77, 0x01, 0x08)         //Consumed resource (0xCF8-0xCFF)
  IO (Decode16, 0xCF8, 0xCF8, 0x01, 0x08)       //Consumed resource (0xCF8-0xCFF)

  WORDIO (                 // Consumed-and-produced resource (all I/O below CF8)
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0000,                // Min
    0x006F,                // Max
    0x0000,                // Translation
    0x0070                 // Range Length
  )

  WORDIO (                 // Consumed-and-produced resource
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0078,                // Min
    0x0CF7,                // Max
    0x0000,                // Translation
    0x0C80                 // Range Length
  )

  WORDIO (                 // Consumed-and-produced resource (all I/O above CFF)
    ResourceProducer,      // bit 0 of general flags is 0
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    PosDecode,
    EntireRange,
    0x0000,                // Granularity
    0x0D00,                // Min
    0xFFFF,                // Max
    0x0000,                // Translation
    0xF300                 // Range Length
  )

  DWORDMEMORY (            // Descriptor for legacy VGA video RAM
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000A0000,            // Min
    0x000BFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for legacy OptionRom
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000C0000,            // Min
    0x000DFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for BIOS Area
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x000E0000,            // Min
    0x000FFFFF,            // Max
    0x00000000,            // Translation
    0x00020000             // Range Length
  )

  DWORDMEMORY (            // Descriptor for ISP0 reserved Mem
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x7A000000,            // Min
    0x7A3FFFFF,            // Max
    0x00000000,            // Translation
    0x00400000             // Range Length
    ,,,
    ISP0
  )

  DWORDMEMORY (            // Descriptor for VGA Stolen Mem
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x7C000000,            // Min
    0x7FFFFFFF,            // Max
    0x00000000,            // Translation
    0x04000000             // Range Length
    ,,,
    STOM
  )

  DWORDMEMORY (            // Descriptor for PCI MMIO
    ResourceProducer,      // bit 0 of general flags is 0
    PosDecode,
    MinFixed,              // Range is fixed
    MaxFixed,              // Range is fixed
    Cacheable,
    ReadWrite,
    0x00000000,            // Granularity
    0x80000000,            // Min
    0xDFFFFFFF,            // Max
    0x00000000,            // Translation
    0x60000000             // Range Length
    ,,,
    PM01
  )
})

//Name(GUID,UUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))
Name(GUID,Buffer()
{
  0x5b, 0x4d, 0xdb, 0x33,
  0xf7, 0x1f,
  0x1c, 0x40,
  0x96, 0x57,
  0x74, 0x41, 0xc0, 0x3d, 0xd7, 0x66
})


Name(SUPP,0)    // PCI _OSC Support Field value
Name(CTRL,0)    // PCI _OSC Control Field value

Method(_OSC,4,Serialized)
{
  // Check for proper UUID
  // Save the capabilities buffer
  Store(Arg3,Local0)

  // Create DWord-adressable fields from the Capabilties Buffer
  CreateDWordField(Local0,0,CDW1)
  CreateDWordField(Local0,4,CDW2)
  CreateDWordField(Local0,8,CDW3)

  // Check for proper UUID
  If(LAnd(LEqual(Arg0,GUID),NEXP))
  {
    // Save Capabilities DWord2 & 3
    Store(CDW2,SUPP)
    Store(CDW3,CTRL)

    If(Not(And(CDW1,1)))    // Query flag clear?
    {
      // Disable GPEs for features granted native control.
      If(And(CTRL,0x02))
      {
        NHPG()
      }
      If(And(CTRL,0x04))      // PME control granted?
      {
        NPME()
      }
    }

    If(LNotEqual(Arg1,One))
    {
      // Unknown revision
      Or(CDW1,0x08,CDW1)
    }

    If(LNotEqual(CDW3,CTRL))
    {
      // Capabilities bits were masked
      Or(CDW1,0x10,CDW1)
    }
    // Update DWORD3 in the buffer
	And(CTRL,0xfe,CTRL)                
    Store(CTRL,CDW3)
    Store(CTRL,OSCC)
    Return(Local0)
  } Else
  {
    Or(CDW1,4,CDW1)         // Unrecognized UUID
    Return(Local0)
  }
}       // End _OSC
