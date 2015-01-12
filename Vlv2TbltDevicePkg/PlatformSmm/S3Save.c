/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

    IchS3Save.c

Abstract:

    SMM S3 handler Driver implementation file

Revision History

**/
#include "SmmPlatform.h"

extern  UINT16                          mAcpiBaseAddr;
EFI_PHYSICAL_ADDRESS                    mRuntimeScriptTableBase;

EFI_STATUS
InitRuntimeScriptTable (
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  UINT32            VarAttrib;
  UINTN             VarSize;
  ACPI_VARIABLE_SET_COMPATIBILITY *AcpiVariableBase;

  //
  // Allocate runtime ACPI script table space. We need it to save some
  // settings done by CSM, which runs after normal script table closed
  //
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIReclaimMemory,
                  1,
                  &mRuntimeScriptTableBase
                  );
  if (EFI_ERROR(Status)) {
    return EFI_OUT_OF_RESOURCES ;
  }

  //
  // Save runtime script table base into global ACPI variable
  //
  VarAttrib = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
              | EFI_VARIABLE_NON_VOLATILE;
  VarSize   = sizeof (UINTN);
  Status = SystemTable->RuntimeServices->GetVariable (
                          ACPI_GLOBAL_VARIABLE,
                          &gEfiAcpiVariableCompatiblityGuid,
                          &VarAttrib,
                          &VarSize,
                          &AcpiVariableBase
                          );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  AcpiVariableBase->RuntimeScriptTableBase = mRuntimeScriptTableBase;

  return EFI_SUCCESS;
}

EFI_STATUS
SaveRuntimeScriptTable (
  VOID
  )
{
  SMM_PCI_IO_ADDRESS    PciAddress;
  UINT32                Data32;
  UINT16                Data16;
  UINT8                 Data8;
  UINT8                 Mask;
  UINTN                 Index;
  UINTN                 Offset;
  UINT8                 RegTable[] = {

	  //
    //Bus  ,   Dev,  Func,    DMI
	  //
      0x00 ,  0x00,  0x00,

	  //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
	  //
      0x00 ,  0x08,  0x00,  0x00,  0x30,  0x00,  0x00,  0xa0,

	  //
    //Bus  ,   Dev,  Func,    LPC device
	  //
      0x00 ,  0x1F,  0x00,

	  //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x00 ,  0x08,  0x00,  0x07,  0x00,  0x00,  0x90,  0x00,

	  //
    //Bus  ,   Dev,  Func,    PCIE device
	 //
      0x00 ,  0x1C,  0x00,

	  //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0xC0 ,  0x83,  0x30,  0x00,  0x00,  0x00,  0x00,  0x00,

	  //
    //Bus  ,   Dev,  Func,    PCIE device
    //
	  0x00 ,  0x1C,  0x00,

	  //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x03 ,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

	  //
    //Bus  ,   Dev,  Func,    SATA device
	  //
      0x00 ,  0x13,  0x00,

	  //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0xf4 ,  0xab,  0x27,  0x10,  0xf1,  0x1d,  0x00,  0x40,

    //
    //Bus  ,   Dev,  Func,    EHCI device
    //
     0x00 ,  0x1D,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
     0x10 ,  0x88,  0x00,  0x00,  0x00,  0x00,  0x00,  0x80,

    //
    //Bus  ,   Dev,  Func,    SMBUS device
    //
     0x00 ,  0x1f,  0x03,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x10 ,  0x89,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    SMBUS device
    //
      0x00 ,  0x1f,  0x03,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x02 ,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    VGA bus1
    //
      0x01 ,  0x00,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x58 ,  0x81,  0x18,  0x01,  0xb0,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    VGA bus1
    //
      0x01 ,  0x00,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x02 ,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    VGA bus1 function 1
    //
      0x01 ,  0x00,  0x01,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x51 ,  0x80,  0x80,  0x01,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    VGA bus1 function 1
    //
      0x01 ,  0x00,  0x01,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x02 ,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    IGD bus0 function 0
    //
      0x00 ,  0x02,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x42 ,  0x81,  0x00,  0x00,  0x00,  0x00,  0x20,  0x00,

    //
    //Bus  ,   Dev,  Func,    USB bus0 function 0
    //
      0x00 ,  0x16,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x32 ,  0x80,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,

    //
    //Bus  ,   Dev,  Func,    HD Audio bus0 function 0
    //
      0x00 ,  0x1B,  0x00,

    //
    //00-1F, 20-3F, 40-5F, 60-7F, 80-9F, A0-BF, C0-DF, E0-FF
    //
      0x00 ,  0x00,  0x00,  0x00,  0x00,  0x00,  0x02,  0x00,

    //
    //0xFF indicates the end of the table
    //
      0xFF
 };

  //
  // These registers have to set in byte order
  //
  UINT8                 ExtReg[] = { 0x9E, 0x9D };  // SMRAM settings



  //
  // Save PCI-Host bridge settings (0, 0, 0). 0x90, 94 and 9c are changed by CSM
  // and vital to S3 resume. That's why we put save code here
  //
  PciAddress.Bus      = 0;
  PciAddress.Device   = 0;
  PciAddress.Function = 0;
  PciAddress.ExtendedRegister = 0;

  for (Index = 0; Index < 2; Index++) {
    //
    // Read SRAM setting from Pci(0, 0, 0)
    //
    PciAddress.Register = ExtReg[Index];
    Data8 = MmioRead8 (
              MmPciAddress (0,
                PciAddress.Bus,
                PciAddress.Device,
                PciAddress.Function,
                PciAddress.Register
              )
            );

    //
    // Save latest settings to runtime script table
    //
    S3BootScriptSavePciCfgWrite(
      S3BootScriptWidthUint8,
      *(UINT64*)&PciAddress,
      1,
      &Data8
      );
  }


  //
  // Save PCI-Host bridge settings (0, 0, 0). 0x90, 94 and 9c are changed by CSM
  // and vital to S3 resume. That's why we put save code here
  //
  Index = 0;
  while (RegTable[Index] != 0xFF) {

    PciAddress.Bus      = RegTable[Index++];
    PciAddress.Device   = RegTable[Index++];
    PciAddress.Function = RegTable[Index++];
    PciAddress.Register = 0;
    PciAddress.ExtendedRegister = 0;

    Data16 = MmioRead16 (
              MmPciAddress (0,
                PciAddress.Bus,
                PciAddress.Device,
                PciAddress.Function,
                PciAddress.Register
              )
            );

    if (Data16 == 0xFFFF) {
      Index+=8;
      continue;
    }

    for (Offset = 0, Mask = 0x01; Offset < 256; Offset+=4, Mask<<=1) {

      if (Mask == 0x00) {
        Mask = 0x01;
      }

      if (RegTable[Index + Offset/32] & Mask ) {

        PciAddress.Register = (UINT8)Offset;
        Data32 = MmioRead32 (MmPciAddress (0, PciAddress.Bus, PciAddress.Device, PciAddress.Function, PciAddress.Register));

        //
        // Save latest settings to runtime script table
        //
        S3BootScriptSavePciCfgWrite (
          S3BootScriptWidthUint32,
          *(UINT64*)&PciAddress,
          1,
          &Data32
        );
      }
    }

    Index += 8;

  }


  //
  // Save I/O ports to S3 script table
  //

  //
  // Selftest KBC
  //
  Data8 = 0xAA;
  S3BootScriptSaveIoWrite (
    S3BootScriptWidthUint8,
    0x64,
    (UINTN)1,
    &Data8
    );

  Data32 = IoRead32(mAcpiBaseAddr + R_PCH_SMI_EN);

  S3BootScriptSaveIoWrite (
      S3BootScriptWidthUint32,
      (mAcpiBaseAddr + R_PCH_SMI_EN),
      1,
      &Data32
      );

  //
  // Save B_ICH_TCO_CNT_LOCK so it will be done on S3 resume path.
  //
  Data16 = IoRead16(mAcpiBaseAddr + R_PCH_TCO_CNT);

  S3BootScriptSaveIoWrite (
      S3BootScriptWidthUint16,
      mAcpiBaseAddr + R_PCH_TCO_CNT,
      1,
      &Data16
      );


  return EFI_SUCCESS;
}
