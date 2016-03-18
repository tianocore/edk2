/** @file
Update the _PRT and _PRW method for pci devices

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/
#include "AcpiPlatform.h"

PCI_DEVICE_INFO *mQNCPciInfo = NULL;

/**
  Init Pci Device Structure
  @param mConfigData    - Pointer of Pci Device information Structure

**/
VOID
InitPciDeviceInfoStructure (
  PCI_DEVICE_SETTING            *mConfigData
  )
{
  //
  // Return 0 given that function unsupported.
  // Would need to parse ACPI tables and build mQNCPciInfo above
  // with found _PRT & _PRW methods for PCI devices.
  //
  mConfigData->PciDeviceInfoNumber = 0;
}

/**
  return Integer value.

  @param Data    - AML data buffer
  @param Integer - integer value.

  @return Data size processed.
**/
UINTN
SdtGetInteger (
  IN  UINT8  *Data,
  OUT UINT64 *Integer
  )
{
  *Integer = 0;
  switch (*Data) {
  case AML_ZERO_OP:
    return 1;
  case AML_ONE_OP:
    *Integer = 1;
    return 1;
  case AML_ONES_OP:
    *Integer = (UINTN)-1;
    return 1;
  case AML_BYTE_PREFIX:
    CopyMem (Integer, Data + 1, sizeof(UINT8));
    return 1 + sizeof(UINT8);
  case AML_WORD_PREFIX:
    CopyMem (Integer, Data + 1, sizeof(UINT16));
    return 1 + sizeof(UINT16);
  case AML_DWORD_PREFIX:
    CopyMem (Integer, Data + 1, sizeof(UINT32));
    return 1 + sizeof(UINT32);
  case AML_QWORD_PREFIX:
    CopyMem (Integer, Data + 1, sizeof(UINT64));
    return 1 + sizeof(UINT64);
  default:
    // Something wrong
    ASSERT (FALSE);
    return 1;
  }
}


/**
  Check if this handle has expected opcode.

  @param AcpiSdt    Pointer to Acpi SDT protocol
  @param Handle     ACPI handle
  @param OpCode     Expected OpCode
  @param SubOpCode  Expected SubOpCode

  @retval TURE  This handle has expected opcode
  @retval FALSE This handle does not have expected opcode
**/
BOOLEAN
SdtIsThisTypeObject (
  IN EFI_ACPI_SDT_PROTOCOL *AcpiSdt,
  IN EFI_ACPI_HANDLE Handle,
  IN UINT8           OpCode,
  IN UINT8           SubOpCode
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;

  Status = AcpiSdt->GetOption (Handle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  if (OpCode == AML_EXT_OP) {
    if (Data[1] == SubOpCode) {
      return TRUE;
    }
  } else {
    if (Data[0] == OpCode) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Check if this handle has expected name and name value.

  @param AcpiSdt    Pointer to Acpi SDT protocol
  @param Handle     ACPI handle
  @param Name       Expected name
  @param Value      Expected name value

  @retval TURE  This handle has expected name and name value.
  @retval FALSE This handle does not have expected name and name value.
**/
BOOLEAN
SdtIsNameIntegerValueEqual (
  IN EFI_ACPI_SDT_PROTOCOL *AcpiSdt,
  IN EFI_ACPI_HANDLE Handle,
  IN CHAR8           *Name,
  IN UINT64          Value
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  UINT64             Integer;

  Status = AcpiSdt->GetOption (Handle, 1, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_NAME_STRING);

  if (CompareMem (Data, Name, 4) != 0) {
    return FALSE;
  }

  //
  // Name match check object
  //
  Status = AcpiSdt->GetOption (Handle, 2, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);

  Integer = 0;
  SdtGetInteger (Data, &Integer);
  if (Integer != Value) {
    return FALSE;
  }

  // All match
  return TRUE;
}

/**
  Check if this handle's children has expected name and name value.

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param ParentHandle     ACPI parent handle
  @param Name             Expected name
  @param Value            Expected name value

  @retval TURE  This handle's children has expected name and name value.
  @retval FALSE This handle's children does not have expected name and name value.
**/
BOOLEAN
SdtCheckNameIntegerValue (
  IN EFI_ACPI_SDT_PROTOCOL *AcpiSdt,
  IN EFI_ACPI_HANDLE ParentHandle,
  IN CHAR8           *Name,
  IN UINT64          Value
  )
{
  EFI_ACPI_HANDLE PreviousHandle;
  EFI_ACPI_HANDLE Handle;
  EFI_STATUS      Status;

  Handle = NULL;
  while (TRUE) {
    PreviousHandle = Handle;
    Status = AcpiSdt->GetChild (ParentHandle, &Handle);
    ASSERT_EFI_ERROR (Status);

    if (PreviousHandle != NULL) {
      Status = AcpiSdt->Close (PreviousHandle);
      ASSERT_EFI_ERROR (Status);
    }

    //
    // Done
    //
    if (Handle == NULL) {
      return FALSE;
    }

    //
    // Check this name
    //
    if (SdtIsThisTypeObject (AcpiSdt, Handle, AML_NAME_OP, 0)) {
      if (SdtIsNameIntegerValueEqual (AcpiSdt, Handle, Name, Value)) {
        return TRUE;
      }
    }
  }

  //
  // Should not run here
  //
}

/**
  Convert the pci address from VPD (bus,dev,fun) into the address that acpi table
  can recognize.

  @param PciAddress    Pci address from VPD

  @retval return the address that acpi table can recognize
**/
UINT32
SdtConvertToAcpiPciAdress (
  IN UINT32 PciAddress
  )
{
  UINT32 ReturnAddress;

  ReturnAddress = ((PciAddress & 0x0000FF00) << 8) | (PciAddress & 0x000000FF);

  if ((PciAddress & 0x000000FF) == 0x000000FF)
    ReturnAddress |= 0x0000FFFF;

  return ReturnAddress;
}

/**
  return AML NameString size.

  @param Buffer - AML name string

  @return AML name string size
**/
UINTN
SdtGetNameStringSize (
  IN UINT8              *Buffer
  )
{
  UINTN                 SegCount;
  UINTN                 Length;
  UINT8                 *Name;

  Name = Buffer;
  Length = 0;

  //
  // Parse root or prefix
  //
  if (*Buffer == AML_ROOT_CHAR) {
    //
    // RootChar
    //
    Buffer ++;
    Length ++;
  } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
    //
    // ParentPrefixChar
    //
    Buffer ++;
    Length ++;
    while (*Buffer == AML_PARENT_PREFIX_CHAR) {
      Buffer ++;
      Length ++;
    }
  }

  //
  // Parse name segment
  //
  if (*Buffer == AML_DUAL_NAME_PREFIX) {
    //
    // DualName
    //
    Buffer ++;
    Length ++;
    SegCount = 2;
  } else if (*Buffer == AML_MULTI_NAME_PREFIX) {
    //
    // MultiName
    //
    Buffer ++;
    Length ++;
    SegCount = *Buffer;
    Buffer ++;
    Length ++;
  } else if (*Buffer == 0) {
    //
    // NULL Name
    //
    SegCount = 0;
    Length ++;
  } else {
    //
    // NameSeg
    //
    SegCount = 1;
  }

  Buffer += 4 * SegCount;
  Length += 4 * SegCount;

  return Length;
}

/**
  The routine to check if this device is PCI root bridge.

  @param AcpiSdt       Pointer to Acpi SDT protocol
  @param DeviceHandle  ACPI device handle
  @param Context       Context info - not used here

  @retval TRUE  This is PCI root bridge
  @retval FALSE This is not PCI root bridge
**/
BOOLEAN
SdtFindRootBridgeHandle (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        CheckHandle,
  IN VOID                   *Context
  )
{
  BOOLEAN            Result;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  EFI_STATUS         Status;

  if (!SdtIsThisTypeObject (AcpiSdt, CheckHandle, AML_EXT_OP, AML_EXT_DEVICE_OP))
    return FALSE;

  Result = SdtCheckNameIntegerValue (AcpiSdt,CheckHandle, "_HID", (UINT64)0x080AD041); // PNP0A08
  if (!Result) {
    Result = SdtCheckNameIntegerValue (AcpiSdt, CheckHandle, "_CID", (UINT64)0x030AD041); // PNP0A03
    if (!Result) {
      return Result;
    }
  }

  //
  // Found
  //
  Status = AcpiSdt->GetOption (CheckHandle, 1, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_NAME_STRING);

  return Result;
}


/**
  The routine to check if this device is wanted.

  @param AcpiSdt       Pointer to Acpi SDT protocol
  @param DeviceHandle  ACPI device handle
  @param Context       Context info - not used here

  @retval TRUE  This is PCI device wanted
  @retval FALSE This is not PCI device wanted
**/
BOOLEAN
SdtFindPciDeviceHandle (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        CheckHandle,
  IN VOID                   *Context
  )
{
  BOOLEAN            Result;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  EFI_STATUS         Status;

  if (!SdtIsThisTypeObject (AcpiSdt, CheckHandle, AML_EXT_OP, AML_EXT_DEVICE_OP))
    return FALSE;

  Result = SdtCheckNameIntegerValue (AcpiSdt,CheckHandle, "_ADR", (UINT64)*(UINT32 *)Context);
  if (!Result) {
    return Result;
  }

  //
  // Found
  //
  Status = AcpiSdt->GetOption (CheckHandle, 1, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_NAME_STRING);

  return Result;
}

/**
  Go through the parent handle and find the handle which pass CheckHandleInfo.

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param ParentHandle     ACPI parent handle
  @param CheckHandleInfo  The callback routine to check if this handle meet the requirement
  @param Context          The context of CheckHandleInfo

  @return the handle which is first one can pass CheckHandleInfo.
**/
EFI_ACPI_HANDLE
SdtGetHandleByScanAllChilds (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        ParentHandle,
  IN CHECK_HANDLE_INFO      CheckHandleInfo,
  IN VOID                   *Context
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_ACPI_HANDLE    Handle;
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    ReturnHandle;

  //
  // Use deep first algo to enumerate all ACPI object
  //
  Handle = NULL;
  while (TRUE) {
    PreviousHandle = Handle;
    Status = AcpiSdt->GetChild (ParentHandle, &Handle);
    ASSERT_EFI_ERROR (Status);

    if (PreviousHandle != NULL) {
      Status = AcpiSdt->Close (PreviousHandle);
      ASSERT_EFI_ERROR (Status);
    }

    //
    // Done
    //
    if (Handle == NULL) {
      return NULL;
    }

    //
    // Check this handle
    //
    if (CheckHandleInfo (AcpiSdt, Handle, Context)) {
      return Handle;
    }

    //
    // Enumerate
    //
    ReturnHandle = SdtGetHandleByScanAllChilds (AcpiSdt, Handle, CheckHandleInfo, Context);
    if (ReturnHandle != NULL) {
      return ReturnHandle;
    }
  }

  //
  // Should not run here
  //
}


/**
  Check whether the INTx package is matched

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param INTxPkgHandle    ACPI INTx package handle
  @param PciAddress       Acpi pci address
  @param INTx             Index of INTx pin
  @param IsAPIC           Tell whether the returned INTx package is for APIC or not

  @retval       TRUE      the INTx package is matched
  @retval       FALSE     the INTx package is not matched

**/
BOOLEAN
SdtCheckINTxPkgIsMatch (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        INTxPkgHandle,
  IN UINT32                 PciAddress,
  IN UINT8                  INTx,
  IN BOOLEAN                *IsAPIC
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    MemberHandle;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  UINT64             CurrentPciAddress;
  UINT64             CurrentINTx;
  UINTN              ChildSize;


  //
  // Check the pci address
  //
  MemberHandle = NULL;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  Status = AcpiSdt->GetOption (MemberHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  CurrentPciAddress = 0;
  SdtGetInteger (Data, &CurrentPciAddress);

  if (CurrentPciAddress != PciAddress) {

    Status = AcpiSdt->Close (MemberHandle);
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  //
  // Check the pci interrupt pin
  //
  PreviousHandle = MemberHandle;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  if (PreviousHandle != NULL) {
    Status = AcpiSdt->Close (PreviousHandle);
    ASSERT_EFI_ERROR (Status);
  }

  Status = AcpiSdt->GetOption (MemberHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  CurrentINTx = 0;
  ChildSize = SdtGetInteger (Data, &CurrentINTx);

  Status = AcpiSdt->Close (MemberHandle);
  ASSERT_EFI_ERROR (Status);

  if (CurrentINTx != INTx)
    return FALSE;

  Data += ChildSize;

  if (*Data == AML_BYTE_PREFIX)
    Data += 1;

  //
  // Check the pci interrupt source
  //
  if (*Data  != 0)
    *IsAPIC = FALSE;
  else
    *IsAPIC = TRUE;

  return TRUE;
}




/**
  Get the wanted INTx package inside the parent package

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param ParentPkgHandle  ACPI parent package handle
  @param PciAddress       Acpi pci address
  @param INTx             Index of INTx pin
  @param INTxPkgHandle    ACPI INTx package handle
  @param IsAPIC           Tell whether the returned INTx package is for APIC or not

**/
VOID
SdtGetINTxPkgHandle (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        ParentPkgHandle,
  IN UINT32                 PciAddress,
  IN UINT8                  INTx,
  IN EFI_ACPI_HANDLE        *INTxPkgHandle,
  IN BOOLEAN                *IsAPIC
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    ChildPkgHandle;

  ChildPkgHandle = NULL;
  while (TRUE) {
    PreviousHandle = ChildPkgHandle;
    Status = AcpiSdt->GetChild (ParentPkgHandle, &ChildPkgHandle);
    ASSERT_EFI_ERROR (Status);

    if (PreviousHandle != NULL) {
      Status = AcpiSdt->Close (PreviousHandle);
      ASSERT_EFI_ERROR (Status);
    }

    if (ChildPkgHandle == NULL) {
      break;
    }

    if (SdtCheckINTxPkgIsMatch(AcpiSdt, ChildPkgHandle, PciAddress, INTx, IsAPIC)) {
      *INTxPkgHandle = ChildPkgHandle;
      return;
    }
  }

  return;
}

/**
  Update the INTx package with the correct pirq value

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param INTxPkgHandle    ACPI INTx package handle
  @param PirqValue        Correct pirq value
  @param IsAPIC           Tell whether the INTx package is for APIC or not

**/
VOID
SdtUpdateINTxPkg (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        INTxPkgHandle,
  IN UINT8                  PirqValue,
  IN BOOLEAN                IsAPIC
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    MemberHandle;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  UINT64             TempValue;
  UINTN              ChildSize;


  //
  // Check the pci address
  //
  MemberHandle = NULL;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  //
  // Check the pci interrupt pin
  //
  PreviousHandle = MemberHandle;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  if (PreviousHandle != NULL) {
    Status = AcpiSdt->Close (PreviousHandle);
    ASSERT_EFI_ERROR (Status);
  }

  Status = AcpiSdt->GetOption (MemberHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  ChildSize = SdtGetInteger (Data, &TempValue);

  Status = AcpiSdt->Close (MemberHandle);
  ASSERT_EFI_ERROR (Status);

  Data += ChildSize;

  //
  // update the pci interrupt source or source index
  //
  if (!IsAPIC) {
    ChildSize = SdtGetNameStringSize (Data);
    Data += (ChildSize - 1);

    PirqValue += 0x40;   // change to ascii char
    if (*Data != PirqValue)
      *Data = PirqValue;
  } else {

    ChildSize = SdtGetInteger (Data, &TempValue);
    Data += ChildSize;

    Data += 1;

    if (*Data != PirqValue)
      *Data = PirqValue;
  }
}

/**
  Check every child package inside this interested parent package for update PRT

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param ParentPkgHandle  ACPI parent package handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
VOID
SdtCheckParentPackage (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        ParentPkgHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_ACPI_HANDLE    INTAPkgHandle;
  EFI_ACPI_HANDLE    INTBPkgHandle;
  EFI_ACPI_HANDLE    INTCPkgHandle;
  EFI_ACPI_HANDLE    INTDPkgHandle;
  UINT32             PciAddress = 0;
  BOOLEAN            IsAllFunctions = FALSE;
  UINT8              IsAPIC = 0;
  EFI_STATUS         Status;

  INTAPkgHandle = INTBPkgHandle = INTCPkgHandle = INTDPkgHandle = NULL;

  PciAddress   = SdtConvertToAcpiPciAdress(PciDeviceInfo->DeviceAddress);

  if ((PciAddress & 0xFFFF) == 0xFFFF) {
    IsAllFunctions = TRUE;
  } else {
    IsAllFunctions = FALSE;
    PciAddress = (PciAddress | 0xFFFF);
  }

  SdtGetINTxPkgHandle (AcpiSdt, ParentPkgHandle, PciAddress, 0, &INTAPkgHandle, (BOOLEAN *)&IsAPIC);
  SdtGetINTxPkgHandle (AcpiSdt, ParentPkgHandle, PciAddress, 1, &INTBPkgHandle, (BOOLEAN *)&IsAPIC);
  SdtGetINTxPkgHandle (AcpiSdt, ParentPkgHandle, PciAddress, 2, &INTCPkgHandle, (BOOLEAN *)&IsAPIC);
  SdtGetINTxPkgHandle (AcpiSdt, ParentPkgHandle, PciAddress, 3, &INTDPkgHandle, (BOOLEAN *)&IsAPIC);

  //
  // Check INTA
  //
  if ((PciDeviceInfo->INTA[IsAPIC] != 0xFF) && (INTAPkgHandle != NULL)) {
    //
    // Find INTA package and there is valid INTA update item, update it
    //
    SdtUpdateINTxPkg (AcpiSdt, INTAPkgHandle, (PciDeviceInfo->INTA[IsAPIC]), IsAPIC);
  } else if ((PciDeviceInfo->INTA[IsAPIC] != 0xFF) && (INTAPkgHandle == NULL)) {
    //
    // There is valid INTA update item, but no INA package exist, should add it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould add INTA item for this device(0x%x)\n\n", PciAddress));

  } else if ((PciDeviceInfo->INTA[IsAPIC] == 0xFF) && (INTAPkgHandle != NULL) && IsAllFunctions) {
    //
    // For all functions senario, if there is invalid INTA update item, but INTA package does exist, should delete it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould remove INTA item for this device(0x%x)\n\n", PciAddress));

  }

  //
  // Check INTB
  //
  if ((PciDeviceInfo->INTB[IsAPIC] != 0xFF) && (INTBPkgHandle != NULL)) {
    //
    // Find INTB package and there is valid INTB update item, update it
    //
    SdtUpdateINTxPkg (AcpiSdt, INTBPkgHandle, (PciDeviceInfo->INTB[IsAPIC]), IsAPIC);
  } else if ((PciDeviceInfo->INTB[IsAPIC] != 0xFF) && (INTBPkgHandle == NULL)) {
    //
    // There is valid INTB update item, but no INTB package exist, should add it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould add INTB item for this device(0x%x)\n\n", PciAddress));

  } else if ((PciDeviceInfo->INTB[IsAPIC] == 0xFF) && (INTBPkgHandle != NULL) && IsAllFunctions) {
    //
    // For all functions senario, if there is invalid INTB update item, but INTB package does exist, should delete it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould remove INTB item for this device(0x%x)\n\n", PciAddress));

  }

  //
  // Check INTC
  //
  if ((PciDeviceInfo->INTC[IsAPIC] != 0xFF) && (INTCPkgHandle != NULL)) {
    //
    // Find INTC package and there is valid INTC update item, update it
    //
    SdtUpdateINTxPkg (AcpiSdt, INTCPkgHandle, (PciDeviceInfo->INTC[IsAPIC]), IsAPIC);
  } else if ((PciDeviceInfo->INTC[IsAPIC] != 0xFF) && (INTCPkgHandle == NULL)) {
    //
    // There is valid INTC update item, but no INTC package exist, should add it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould add INTC item for this device(0x%x)\n\n", PciAddress));

  } else if ((PciDeviceInfo->INTC[IsAPIC] == 0xFF) && (INTCPkgHandle != NULL) && IsAllFunctions) {
    //
    // For all functions senario, if there is invalid INTC update item, but INTC package does exist, should delete it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould remove INTC item for this device(0x%x)\n\n", PciAddress));
  }

  //
  // Check INTD
  //
  if ((PciDeviceInfo->INTD[IsAPIC] != 0xFF) && (INTDPkgHandle != NULL)) {
    //
    // Find INTD package and there is valid INTD update item, update it
    //
    SdtUpdateINTxPkg (AcpiSdt, INTDPkgHandle, (PciDeviceInfo->INTD[IsAPIC]), IsAPIC);
  } else if ((PciDeviceInfo->INTD[IsAPIC] != 0xFF) && (INTDPkgHandle == NULL)) {
    //
    // There is valid INTD update item, but no INTD package exist, should add it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould add INTD item for this device(0x%x)\n\n", PciAddress));

  }  else if ((PciDeviceInfo->INTD[IsAPIC] == 0xFF) && (INTDPkgHandle != NULL) && IsAllFunctions) {
    //
    // For all functions senario, if there is invalid INTD update item, but INTD package does exist, should delete it
    //
    DEBUG ((EFI_D_ERROR, "\n\nShould remove INTD item for this device(0x%x)\n\n", PciAddress));
  }


  if (INTAPkgHandle != NULL) {
    Status = AcpiSdt->Close (INTAPkgHandle);
    ASSERT_EFI_ERROR (Status);
  }

  if (INTBPkgHandle != NULL) {
    Status = AcpiSdt->Close (INTBPkgHandle);
    ASSERT_EFI_ERROR (Status);
  }

  if (INTCPkgHandle != NULL) {
    Status = AcpiSdt->Close (INTCPkgHandle);
    ASSERT_EFI_ERROR (Status);
  }

  if (INTDPkgHandle != NULL) {
    Status = AcpiSdt->Close (INTDPkgHandle);
    ASSERT_EFI_ERROR (Status);
  }

  return;
}

/**
  Check every return package for update PRT

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param ParentHandle     ACPI pci device handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
VOID
SdtCheckReturnPackage (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        MethodHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_ACPI_HANDLE    ReturnHandle;
  EFI_ACPI_HANDLE    PackageHandle;
  EFI_ACPI_HANDLE    NamePkgHandle;
  EFI_STATUS         Status;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;
  CHAR8              NameStr[128];

  ReturnHandle = NULL;
  while (TRUE) {
    PreviousHandle = ReturnHandle;
    Status = AcpiSdt->GetChild (MethodHandle, &ReturnHandle);
    ASSERT_EFI_ERROR (Status);

    if (PreviousHandle != NULL) {
      Status = AcpiSdt->Close (PreviousHandle);
      ASSERT_EFI_ERROR (Status);
    }

    if (ReturnHandle == NULL) {
      break;
    }

    Status = AcpiSdt->GetOption (ReturnHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
    ASSERT_EFI_ERROR (Status);
    ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

    if (*Data == AML_RETURN_OP) {
      //
      // Find the return method handle, then look for the returned package data
      //
      Status = AcpiSdt->GetOption (ReturnHandle, 1, &DataType, (CONST VOID **)&Data, &DataSize);
      ASSERT_EFI_ERROR (Status);


      if (DataType == EFI_ACPI_DATA_TYPE_NAME_STRING) {
        ZeroMem (NameStr, 128);
        AsciiStrCpy (NameStr, "\\_SB.");
        DataSize = SdtGetNameStringSize (Data);
        AsciiStrnCat (NameStr, (CHAR8 *)Data, DataSize);

        NamePkgHandle = NULL;
        Status = AcpiSdt->FindPath (mDsdtHandle, NameStr, &NamePkgHandle);
        ASSERT_EFI_ERROR (Status);
        ASSERT (NamePkgHandle != NULL);

        Status = AcpiSdt->GetOption (NamePkgHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
        ASSERT_EFI_ERROR (Status);
        ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);
        ASSERT (*Data == AML_NAME_OP);

        Status = AcpiSdt->GetOption (NamePkgHandle, 2, &DataType, (CONST VOID **)&Data, &DataSize);
        ASSERT_EFI_ERROR (Status);
        ASSERT (DataType == EFI_ACPI_DATA_TYPE_CHILD);
      }

      ASSERT (DataType == EFI_ACPI_DATA_TYPE_CHILD);

      //
      // Get the parent package handle
      //
      PackageHandle = NULL;
      Status = AcpiSdt->Open (Data, &PackageHandle);
      ASSERT_EFI_ERROR (Status);

      //
      // Check the parent package for update pci routing
      //
      SdtCheckParentPackage (AcpiSdt, PackageHandle, PciDeviceInfo);

      Status = AcpiSdt->Close (PackageHandle);
      ASSERT_EFI_ERROR (Status);

      Status = AcpiSdt->Close (ReturnHandle);
      ASSERT_EFI_ERROR (Status);

      break;
    }

    //
    // Not ReturnOp, search it as parent
    //
    SdtCheckReturnPackage (AcpiSdt, ReturnHandle, PciDeviceInfo);
  }

  //
  // Done
  //
  return;

}

/**
  update interrupt info inside the PRT method for the given pci device handle

  @param AcpiSdt       Pointer to Acpi SDT protocol
  @param PciHandle     ACPI pci device handle
  @param PciDeviceInfo Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePrtMethod (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        PciHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    PrtMethodHandle;

  //
  // Find the PRT method under this pci device
  //
  PrtMethodHandle = NULL;
  Status = AcpiSdt->FindPath (PciHandle, "_PRT", &PrtMethodHandle);

  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (PrtMethodHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SdtCheckReturnPackage(AcpiSdt, PrtMethodHandle, PciDeviceInfo);

  Status = AcpiSdt->Close (PrtMethodHandle);
  ASSERT_EFI_ERROR (Status);

  return Status;
}


/**
  Update the package inside name op with correct wakeup resources

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param InPkgHandle      ACPI inside package handle
  @param GPEPin           Correct gpe pin
  @param SxNum            Correct system state the device can wake up from

**/
VOID
SdtUpdatePackageInName (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        INTxPkgHandle,
  IN UINT8                  GPEPin,
  IN UINT8                  SxNum
  )
{
  EFI_ACPI_HANDLE    PreviousHandle;
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    MemberHandle;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;

  //
  // Check the gpe pin
  //
  MemberHandle = NULL;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  Status = AcpiSdt->GetOption (MemberHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  //
  // Skip byte prefix
  //
  Data += 1;

  if (*Data != GPEPin) {

    *Data = GPEPin;
  }

  //
  // Check the sx number
  //
  PreviousHandle = MemberHandle;
  Status = AcpiSdt->GetChild (INTxPkgHandle, &MemberHandle);
  ASSERT_EFI_ERROR (Status);
  ASSERT (MemberHandle != NULL);

  if (PreviousHandle != NULL) {
    Status = AcpiSdt->Close (PreviousHandle);
    ASSERT_EFI_ERROR (Status);
  }

  Status = AcpiSdt->GetOption (MemberHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);

  //
  // Skip byte prefix
  //
  Data += 1;

  if (*Data != SxNum) {

    *Data = SxNum;
  }

  Status = AcpiSdt->Close (MemberHandle);
  ASSERT_EFI_ERROR (Status);

}

/**
  Check the name package belonged to PRW

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param PrwPkgHandle     ACPI PRW package handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
VOID
SdtCheckNamePackage (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        PrwPkgHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_ACPI_HANDLE    InPkgHandle;
  EFI_STATUS         Status;
  EFI_ACPI_DATA_TYPE DataType;
  UINT8              *Data;
  UINTN              DataSize;

  Status = AcpiSdt->GetOption (PrwPkgHandle, 0, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_OPCODE);
  ASSERT (*Data == AML_NAME_OP);

  Status = AcpiSdt->GetOption (PrwPkgHandle, 2, &DataType, (CONST VOID **)&Data, &DataSize);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataType == EFI_ACPI_DATA_TYPE_CHILD);

  //
  // Get the inside package handle
  //
  InPkgHandle = NULL;
  Status = AcpiSdt->Open (Data, &InPkgHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // update the package in name op for wakeup info
  //
  if ((PciDeviceInfo->GPEPin != 0xFF) && (PciDeviceInfo->SxNum != 0xFF))
    SdtUpdatePackageInName (AcpiSdt, InPkgHandle, PciDeviceInfo->GPEPin, PciDeviceInfo->SxNum);

  Status = AcpiSdt->Close (InPkgHandle);
  ASSERT_EFI_ERROR (Status);

  return;

}

/**
  update wakeup info inside the PRW method for the given pci device handle

  @param AcpiSdt       Pointer to Acpi SDT protocol
  @param PciHandle     ACPI pci device handle
  @param PciDeviceInfo Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePrwPackage (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        PciHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    PrwPkgHandle;

  //
  // Find the PRT method under this pci device
  //
  PrwPkgHandle = NULL;
  Status = AcpiSdt->FindPath (PciHandle, "_PRW", &PrwPkgHandle);

  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (PrwPkgHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SdtCheckNamePackage(AcpiSdt, PrwPkgHandle, PciDeviceInfo);

  Status = AcpiSdt->Close (PrwPkgHandle);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  update pci routing information in acpi table based on pcd settings

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param PciRootHandle       ACPI root bridge handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePciRouting (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        PciRootHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    PciBridgeHandle;
  UINT32             PciAddress;


  PciBridgeHandle = NULL;
  if (PciDeviceInfo->BridgeAddress == 0x00000000) {
    //
    // Its bridge is the host root bridge
    //
    PciBridgeHandle = PciRootHandle;

  } else {

    //
    // Its bridge is just a pci device under the host bridge
    //

    //
    // Conver the bridge address into one that acpi table can recognize
    //
    PciAddress = SdtConvertToAcpiPciAdress (PciDeviceInfo->BridgeAddress);

    //
    // Scan the whole table to find the pci device
    //
    PciBridgeHandle = SdtGetHandleByScanAllChilds(AcpiSdt, PciRootHandle, SdtFindPciDeviceHandle, &PciAddress);
    if (PciBridgeHandle == NULL) {

      return EFI_INVALID_PARAMETER;
    }
  }

  Status = SdtUpdatePrtMethod(AcpiSdt, PciBridgeHandle, PciDeviceInfo);

  if (PciDeviceInfo->BridgeAddress != 0x00000000) {
    Status = AcpiSdt->Close (PciBridgeHandle);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}


/**
  update power resource wake up information in acpi table based on pcd settings

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param PciRootHandle    ACPI root bridge handle
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO

**/
EFI_STATUS
SdtUpdatePowerWake (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        PciRootHandle,
  IN PCI_DEVICE_INFO        *PciDeviceInfo
  )
{
  EFI_STATUS         Status;
  EFI_ACPI_HANDLE    PciBridgeHandle;
  EFI_ACPI_HANDLE    PciDeviceHandle;
  UINT32             PciAddress;

  PciBridgeHandle = NULL;
  if (PciDeviceInfo->BridgeAddress == 0x00000000) {
    //
    // Its bridge is the host root bridge
    //
    PciBridgeHandle = PciRootHandle;

  } else {

    //
    // Its bridge is just a pci device under the host bridge
    //

    //
    // Conver the bridge address into one that acpi table can recognize
    //
    PciAddress = SdtConvertToAcpiPciAdress (PciDeviceInfo->BridgeAddress);

    //
    // Scan the whole table to find the pci device
    //
    PciBridgeHandle = SdtGetHandleByScanAllChilds(AcpiSdt, PciRootHandle, SdtFindPciDeviceHandle, &PciAddress);

    if (PciBridgeHandle == NULL) {

      Status = AcpiSdt->Close (PciRootHandle);
      ASSERT_EFI_ERROR (Status);

      return EFI_INVALID_PARAMETER;
    }
  }

  PciDeviceHandle = NULL;

  //
  // Conver the device address into one that acpi table can recognize
  //
  PciAddress = SdtConvertToAcpiPciAdress (PciDeviceInfo->DeviceAddress);

  //
  // Scan the whole table to find the pci device
  //
  PciDeviceHandle = SdtGetHandleByScanAllChilds(AcpiSdt, PciBridgeHandle, SdtFindPciDeviceHandle, &PciAddress);

  if (PciDeviceHandle == NULL) {
    if (PciDeviceInfo->BridgeAddress != 0x00000000) {
      Status = AcpiSdt->Close (PciBridgeHandle);
      ASSERT_EFI_ERROR (Status);
    }

    return EFI_INVALID_PARAMETER;
  }

  Status = SdtUpdatePrwPackage(AcpiSdt, PciDeviceHandle, PciDeviceInfo);

  Status = AcpiSdt->Close (PciDeviceHandle);
  ASSERT_EFI_ERROR (Status);

  if (PciDeviceInfo->BridgeAddress != 0x00000000) {
    Status = AcpiSdt->Close (PciBridgeHandle);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}


/**
  Get the root bridge handle by scanning the acpi table

  @param AcpiSdt          Pointer to Acpi SDT protocol
  @param DsdtHandle       ACPI root handle

  @retval EFI_ACPI_HANDLE the handle of the root bridge
**/
EFI_ACPI_HANDLE
SdtGetRootBridgeHandle (
  IN EFI_ACPI_SDT_PROTOCOL  *AcpiSdt,
  IN EFI_ACPI_HANDLE        DsdtHandle
  )
{
  EFI_ACPI_HANDLE    PciRootHandle;

  //
  // Scan the whole table to find the root bridge
  //
  PciRootHandle = NULL;
  PciRootHandle = SdtGetHandleByScanAllChilds(AcpiSdt, DsdtHandle, SdtFindRootBridgeHandle, NULL);
  ASSERT (PciRootHandle != NULL);

  return PciRootHandle;
}


/**
  Check input Pci device info is changed from the default values
  @param PciDeviceInfo    Pointer to PCI_DEVICE_INFO
  @param UpdatePRT        Pointer to BOOLEAN
  @param UpdatePRW        Pointer to BOOLEAN

**/
VOID
SdtCheckPciDeviceInfoChanged (
  IN PCI_DEVICE_INFO        *PciDeviceInfo,
  IN BOOLEAN                *UpdatePRT,
  IN BOOLEAN                *UpdatePRW
  )
{
  UINTN Index = 0;

  if (mQNCPciInfo == NULL) {
    *UpdatePRT = FALSE;
    *UpdatePRW = FALSE;
    return;
  }

  *UpdatePRT = TRUE;
  *UpdatePRW = TRUE;

  for (Index = 0;Index < CURRENT_PCI_DEVICE_NUM; Index++) {
    if ((mQNCPciInfo[Index].BridgeAddress == PciDeviceInfo->BridgeAddress)
      && (mQNCPciInfo[Index].DeviceAddress == PciDeviceInfo->DeviceAddress)) {
      //
      // Find one matched entry
      //
      if (CompareMem (&(mQNCPciInfo[Index].INTA[0]), &PciDeviceInfo->INTA[0], 10) == 0) {
        *UpdatePRT = FALSE;
        *UpdatePRW = FALSE;
        //DEBUG ((EFI_D_ERROR, "Find one matched entry[%d] and no change\n", Index));
      } else {
        if (CompareMem (&(mQNCPciInfo[Index].INTA[0]), &PciDeviceInfo->INTA[0], 8) == 0)
          *UpdatePRT = FALSE;

        if (CompareMem (&(mQNCPciInfo[Index].GPEPin), &PciDeviceInfo->GPEPin, 2) == 0)
          *UpdatePRW = FALSE;

        if (*(UINT64 *)(&PciDeviceInfo->INTA[0]) == 0xFFFFFFFFFFFFFFFFULL)
          *UpdatePRT = FALSE;

        if (*(UINT16 *)(&PciDeviceInfo->GPEPin) == 0xFFFF)
          *UpdatePRW = FALSE;

        //DEBUG ((EFI_D_ERROR, "Find one matched entry[%d] and but need update PRT:0x%x PRW:0x%x\n", Index, *UpdatePRT, *UpdatePRW));
      }
      break;
    }
  }

  //if (Index == 42) {
  //  DEBUG ((EFI_D_ERROR, "Find No matched entry\n"));
  //}

  return;
}
