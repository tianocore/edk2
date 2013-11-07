/** @file
  DevicePathFromText protocol as defined in the UEFI 2.0 specification.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiDevicePathLib.h"

/**

  Duplicates a string.

  @param  Src  Source string.

  @return The duplicated string.

**/
CHAR16 *
UefiDevicePathLibStrDuplicate (
  IN CONST CHAR16  *Src
  )
{
  return AllocateCopyPool (StrSize (Src), Src);
}

/**

  Get parameter in a pair of parentheses follow the given node name.
  For example, given the "Pci(0,1)" and NodeName "Pci", it returns "0,1".

  @param  Str      Device Path Text.
  @param  NodeName Name of the node.

  @return Parameter text for the node.

**/
CHAR16 *
GetParamByNodeName (
  IN CHAR16 *Str,
  IN CHAR16 *NodeName
  )
{
  CHAR16  *ParamStr;
  CHAR16  *StrPointer;
  UINTN   NodeNameLength;
  UINTN   ParameterLength;

  //
  // Check whether the node name matchs
  //
  NodeNameLength = StrLen (NodeName);
  if (StrnCmp (Str, NodeName, NodeNameLength) != 0) {
    return NULL;
  }

  ParamStr = Str + NodeNameLength;
  if (!IS_LEFT_PARENTH (*ParamStr)) {
    return NULL;
  }

  //
  // Skip the found '(' and find first occurrence of ')'
  //
  ParamStr++;
  ParameterLength = 0;
  StrPointer = ParamStr;
  while (!IS_NULL (*StrPointer)) {
    if (IS_RIGHT_PARENTH (*StrPointer)) {
      break;
    }
    StrPointer++;
    ParameterLength++;
  }
  if (IS_NULL (*StrPointer)) {
    //
    // ')' not found
    //
    return NULL;
  }

  ParamStr = AllocateCopyPool ((ParameterLength + 1) * sizeof (CHAR16), ParamStr);
  if (ParamStr == NULL) {
    return NULL;
  }
  //
  // Terminate the parameter string
  //
  ParamStr[ParameterLength] = L'\0';

  return ParamStr;
}

/**
  Gets current sub-string from a string list, before return
  the list header is moved to next sub-string. The sub-string is separated
  by the specified character. For example, the separator is ',', the string
  list is "2,0,3", it returns "2", the remain list move to "0,3"

  @param  List        A string list separated by the specified separator
  @param  Separator   The separator character

  @return A pointer to the current sub-string

**/
CHAR16 *
SplitStr (
  IN OUT CHAR16 **List,
  IN     CHAR16 Separator
  )
{
  CHAR16  *Str;
  CHAR16  *ReturnStr;

  Str = *List;
  ReturnStr = Str;

  if (IS_NULL (*Str)) {
    return ReturnStr;
  }

  //
  // Find first occurrence of the separator
  //
  while (!IS_NULL (*Str)) {
    if (*Str == Separator) {
      break;
    }
    Str++;
  }

  if (*Str == Separator) {
    //
    // Find a sub-string, terminate it
    //
    *Str = L'\0';
    Str++;
  }

  //
  // Move to next sub-string
  //
  *List = Str;

  return ReturnStr;
}

/**
  Gets the next parameter string from the list.

  @param List            A string list separated by the specified separator

  @return A pointer to the current sub-string

**/
CHAR16 *
GetNextParamStr (
  IN OUT CHAR16 **List
  )
{
  //
  // The separator is comma
  //
  return SplitStr (List, L',');
}

/**
  Get one device node from entire device path text.

  @param DevicePath      On input, the current Device Path node; on output, the next device path node
  @param IsInstanceEnd   This node is the end of a device path instance

  @return A device node text or NULL if no more device node available

**/
CHAR16 *
GetNextDeviceNodeStr (
  IN OUT CHAR16   **DevicePath,
  OUT    BOOLEAN  *IsInstanceEnd
  )
{
  CHAR16  *Str;
  CHAR16  *ReturnStr;
  UINTN   ParenthesesStack;

  Str = *DevicePath;
  if (IS_NULL (*Str)) {
    return NULL;
  }

  //
  // Skip the leading '/', '(', ')' and ','
  //
  while (!IS_NULL (*Str)) {
    if (!IS_SLASH (*Str) &&
        !IS_COMMA (*Str) &&
        !IS_LEFT_PARENTH (*Str) &&
        !IS_RIGHT_PARENTH (*Str)) {
      break;
    }
    Str++;
  }

  ReturnStr = Str;

  //
  // Scan for the separator of this device node, '/' or ','
  //
  ParenthesesStack = 0;
  while (!IS_NULL (*Str)) {
    if ((IS_COMMA (*Str) || IS_SLASH (*Str)) && (ParenthesesStack == 0)) {
      break;
    }

    if (IS_LEFT_PARENTH (*Str)) {
      ParenthesesStack++;
    } else if (IS_RIGHT_PARENTH (*Str)) {
      ParenthesesStack--;
    }

    Str++;
  }

  if (ParenthesesStack != 0) {
    //
    // The '(' doesn't pair with ')', invalid device path text
    //
    return NULL;
  }

  if (IS_COMMA (*Str)) {
    *IsInstanceEnd = TRUE;
    *Str = L'\0';
    Str++;
  } else {
    *IsInstanceEnd = FALSE;
    if (!IS_NULL (*Str)) {
      *Str = L'\0';
      Str++;
    }
  }

  *DevicePath = Str;

  return ReturnStr;
}


/**
  Return whether the integer string is a hex string.

  @param Str             The integer string

  @retval TRUE   Hex string
  @retval FALSE  Decimal string

**/
BOOLEAN
IsHexStr (
  IN CHAR16   *Str
  )
{
  //
  // skip preceeding white space
  //
  while ((*Str != 0) && *Str == L' ') {
    Str ++;
  }
  //
  // skip preceeding zeros
  //
  while ((*Str != 0) && *Str == L'0') {
    Str ++;
  }
  
  return (BOOLEAN) (*Str == L'x' || *Str == L'X');
}

/**

  Convert integer string to uint.

  @param Str             The integer string. If leading with "0x" or "0X", it's hexadecimal.

  @return A UINTN value represented by Str

**/
UINTN
Strtoi (
  IN CHAR16  *Str
  )
{
  if (IsHexStr (Str)) {
    return StrHexToUintn (Str);
  } else {
    return StrDecimalToUintn (Str);
  }
}

/**

  Convert integer string to 64 bit data.

  @param Str             The integer string. If leading with "0x" or "0X", it's hexadecimal.
  @param Data            A pointer to the UINT64 value represented by Str

**/
VOID
Strtoi64 (
  IN  CHAR16  *Str,
  OUT UINT64  *Data
  )
{
  if (IsHexStr (Str)) {
    *Data = StrHexToUint64 (Str);
  } else {
    *Data = StrDecimalToUint64 (Str);
  }
}

/**
  Converts a list of string to a specified buffer.

  @param Buf             The output buffer that contains the string.
  @param BufferLength    The length of the buffer
  @param Str             The input string that contains the hex number

  @retval EFI_SUCCESS    The string was successfully converted to the buffer.

**/
EFI_STATUS
StrToBuf (
  OUT UINT8    *Buf,
  IN  UINTN    BufferLength,
  IN  CHAR16   *Str
  )
{
  UINTN       Index;
  UINTN       StrLength;
  UINT8       Digit;
  UINT8       Byte;

  Digit = 0;

  //
  // Two hex char make up one byte
  //
  StrLength = BufferLength * sizeof (CHAR16);

  for(Index = 0; Index < StrLength; Index++, Str++) {

    if ((*Str >= L'a') && (*Str <= L'f')) {
      Digit = (UINT8) (*Str - L'a' + 0x0A);
    } else if ((*Str >= L'A') && (*Str <= L'F')) {
      Digit = (UINT8) (*Str - L'A' + 0x0A);
    } else if ((*Str >= L'0') && (*Str <= L'9')) {
      Digit = (UINT8) (*Str - L'0');
    } else {
      return EFI_INVALID_PARAMETER;
    }

    //
    // For odd characters, write the upper nibble for each buffer byte,
    // and for even characters, the lower nibble.
    //
    if ((Index & 1) == 0) {
      Byte = (UINT8) (Digit << 4);
    } else {
      Byte = Buf[Index / 2];
      Byte &= 0xF0;
      Byte = (UINT8) (Byte | Digit);
    }

    Buf[Index / 2] = Byte;
  }

  return EFI_SUCCESS;
}

/**
  Converts a string to GUID value.
  Guid Format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

  @param Str              The registry format GUID string that contains the GUID value.
  @param Guid             A pointer to the converted GUID value.

  @retval EFI_SUCCESS     The GUID string was successfully converted to the GUID value.
  @retval EFI_UNSUPPORTED The input string is not in registry format.
  @return others          Some error occurred when converting part of GUID value.

**/
EFI_STATUS
StrToGuid (
  IN  CHAR16   *Str,
  OUT EFI_GUID *Guid
  )
{
  //
  // Get the first UINT32 data
  //
  Guid->Data1 = (UINT32) StrHexToUint64  (Str);
  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }
  
  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Get the second UINT16 data
  //
  Guid->Data2 = (UINT16) StrHexToUint64  (Str);
  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Get the third UINT16 data
  //
  Guid->Data3 = (UINT16) StrHexToUint64  (Str);
  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the following 8 bytes data
  //  
  StrToBuf (&Guid->Data4[0], 2, Str);
  //
  // Skip 2 byte hex chars
  //
  Str += 2 * 2;

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }
  StrToBuf (&Guid->Data4[2], 6, Str);

  return EFI_SUCCESS;
}

/**
  Converts a string to IPv4 address

  @param Str             A string representation of IPv4 address.
  @param IPv4Addr        A pointer to the converted IPv4 address.

**/
VOID
StrToIPv4Addr (
  IN OUT CHAR16           **Str,
  OUT    EFI_IPv4_ADDRESS *IPv4Addr
  )
{
  UINTN  Index;

  for (Index = 0; Index < 4; Index++) {
    IPv4Addr->Addr[Index] = (UINT8) Strtoi (SplitStr (Str, L'.'));
  }
}

/**
  Converts a string to IPv4 address

  @param Str             A string representation of IPv6 address.
  @param IPv6Addr        A pointer to the converted IPv6 address.

**/
VOID
StrToIPv6Addr (
  IN OUT CHAR16           **Str,
  OUT    EFI_IPv6_ADDRESS *IPv6Addr
  )
{
  UINTN  Index;
  UINT16 Data;

  for (Index = 0; Index < 8; Index++) {
    Data = (UINT16) StrHexToUintn (SplitStr (Str, L':'));
    IPv6Addr->Addr[Index * 2] = (UINT8) (Data >> 8);
    IPv6Addr->Addr[Index * 2 + 1] = (UINT8) (Data & 0xff);
  }
}

/**
  Converts a Unicode string to ASCII string.

  @param Str             The equivalent Unicode string
  @param AsciiStr        On input, it points to destination ASCII string buffer; on output, it points
                         to the next ASCII string next to it

**/
VOID
StrToAscii (
  IN     CHAR16 *Str,
  IN OUT CHAR8  **AsciiStr
  )
{
  CHAR8 *Dest;

  Dest = *AsciiStr;
  while (!IS_NULL (*Str)) {
    *(Dest++) = (CHAR8) *(Str++);
  }
  *Dest = 0;

  //
  // Return the string next to it
  //
  *AsciiStr = Dest + 1;
}

/**
  Converts a text device path node to Hardware PCI device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to Hardware PCI device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextPci (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16          *FunctionStr;
  CHAR16          *DeviceStr;
  PCI_DEVICE_PATH *Pci;

  DeviceStr   = GetNextParamStr (&TextDeviceNode);
  FunctionStr = GetNextParamStr (&TextDeviceNode);
  Pci         = (PCI_DEVICE_PATH *) CreateDeviceNode (
                                      HARDWARE_DEVICE_PATH,
                                      HW_PCI_DP,
                                      (UINT16) sizeof (PCI_DEVICE_PATH)
                                      );

  Pci->Function = (UINT8) Strtoi (FunctionStr);
  Pci->Device   = (UINT8) Strtoi (DeviceStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Pci;
}

/**
  Converts a text device path node to Hardware PC card device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to Hardware PC card device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextPcCard (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16              *FunctionNumberStr;
  PCCARD_DEVICE_PATH  *Pccard;

  FunctionNumberStr = GetNextParamStr (&TextDeviceNode);
  Pccard            = (PCCARD_DEVICE_PATH *) CreateDeviceNode (
                                               HARDWARE_DEVICE_PATH,
                                               HW_PCCARD_DP,
                                               (UINT16) sizeof (PCCARD_DEVICE_PATH)
                                               );

  Pccard->FunctionNumber  = (UINT8) Strtoi (FunctionNumberStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Pccard;
}

/**
  Converts a text device path node to Hardware memory map device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to Hardware memory map device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextMemoryMapped (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16              *MemoryTypeStr;
  CHAR16              *StartingAddressStr;
  CHAR16              *EndingAddressStr;
  MEMMAP_DEVICE_PATH  *MemMap;

  MemoryTypeStr      = GetNextParamStr (&TextDeviceNode);
  StartingAddressStr = GetNextParamStr (&TextDeviceNode);
  EndingAddressStr   = GetNextParamStr (&TextDeviceNode);
  MemMap             = (MEMMAP_DEVICE_PATH *) CreateDeviceNode (
                                               HARDWARE_DEVICE_PATH,
                                               HW_MEMMAP_DP,
                                               (UINT16) sizeof (MEMMAP_DEVICE_PATH)
                                               );

  MemMap->MemoryType = (UINT32) Strtoi (MemoryTypeStr);
  Strtoi64 (StartingAddressStr, &MemMap->StartingAddress);
  Strtoi64 (EndingAddressStr, &MemMap->EndingAddress);

  return (EFI_DEVICE_PATH_PROTOCOL *) MemMap;
}

/**
  Converts a text device path node to Vendor device path structure based on the input Type
  and SubType.

  @param TextDeviceNode  The input Text device path node.
  @param Type            The type of device path node.
  @param SubType         The subtype of device path node.

  @return A pointer to the newly-created Vendor device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
ConvertFromTextVendor (
  IN CHAR16 *TextDeviceNode,
  IN UINT8  Type,
  IN UINT8  SubType
  )
{
  CHAR16              *GuidStr;
  CHAR16              *DataStr;
  UINTN               Length;
  VENDOR_DEVICE_PATH  *Vendor;

  GuidStr = GetNextParamStr (&TextDeviceNode);

  DataStr = GetNextParamStr (&TextDeviceNode);
  Length  = StrLen (DataStr);
  //
  // Two hex characters make up 1 buffer byte
  //
  Length  = (Length + 1) / 2;

  Vendor  = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                     Type,
                                     SubType,
                                     (UINT16) (sizeof (VENDOR_DEVICE_PATH) + Length)
                                     );

  StrToGuid (GuidStr, &Vendor->Guid);
  StrToBuf (((UINT8 *) Vendor) + sizeof (VENDOR_DEVICE_PATH), Length, DataStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Vendor Hardware device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor Hardware device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenHw (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextVendor (
           TextDeviceNode,
           HARDWARE_DEVICE_PATH,
           HW_VENDOR_DP
           );
}

/**
  Converts a text device path node to Hardware Controller device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Hardware Controller device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextCtrl (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                  *ControllerStr;
  CONTROLLER_DEVICE_PATH  *Controller;

  ControllerStr = GetNextParamStr (&TextDeviceNode);
  Controller    = (CONTROLLER_DEVICE_PATH *) CreateDeviceNode (
                                               HARDWARE_DEVICE_PATH,
                                               HW_CONTROLLER_DP,
                                               (UINT16) sizeof (CONTROLLER_DEVICE_PATH)
                                               );
  Controller->ControllerNumber = (UINT32) Strtoi (ControllerStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Controller;
}

/**
  Converts a string to EisaId.

  @param Text   The input string.

  @return UINT32 EISA ID.
**/
UINT32
EisaIdFromText (
  IN CHAR16 *Text
  )
{
  return (((Text[0] - 'A' + 1) & 0x1f) << 10)
       + (((Text[1] - 'A' + 1) & 0x1f) <<  5)
       + (((Text[2] - 'A' + 1) & 0x1f) <<  0)
       + (UINT32) (StrHexToUintn (&Text[3]) << 16)
       ;
}

/**
  Converts a text device path node to ACPI HID device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created ACPI HID device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextAcpi (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                *HIDStr;
  CHAR16                *UIDStr;
  ACPI_HID_DEVICE_PATH  *Acpi;

  HIDStr = GetNextParamStr (&TextDeviceNode);
  UIDStr = GetNextParamStr (&TextDeviceNode);
  Acpi   = (ACPI_HID_DEVICE_PATH *) CreateDeviceNode (
                                      ACPI_DEVICE_PATH,
                                      ACPI_DP,
                                      (UINT16) sizeof (ACPI_HID_DEVICE_PATH)
                                      );

  Acpi->HID = EisaIdFromText (HIDStr);
  Acpi->UID = (UINT32) Strtoi (UIDStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Acpi;
}

/**
  Converts a text device path node to ACPI HID device path structure.

  @param TextDeviceNode  The input Text device path node.
  @param PnPId           The input plug and play identification.

  @return A pointer to the newly-created ACPI HID device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
ConvertFromTextAcpi (
  IN CHAR16 *TextDeviceNode,
  IN UINT32  PnPId
  )
{
  CHAR16                *UIDStr;
  ACPI_HID_DEVICE_PATH  *Acpi;

  UIDStr = GetNextParamStr (&TextDeviceNode);
  Acpi   = (ACPI_HID_DEVICE_PATH *) CreateDeviceNode (
                                      ACPI_DEVICE_PATH,
                                      ACPI_DP,
                                      (UINT16) sizeof (ACPI_HID_DEVICE_PATH)
                                      );

  Acpi->HID = EFI_PNP_ID (PnPId);
  Acpi->UID = (UINT32) Strtoi (UIDStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Acpi;
}

/**
  Converts a text device path node to PCI root device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created PCI root device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextPciRoot (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0a03);
}

/**
  Converts a text device path node to PCIE root device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created PCIE root device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextPcieRoot (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0a08);
}

/**
  Converts a text device path node to Floppy device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Floppy device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFloppy (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0604);
}

/**
  Converts a text device path node to Keyboard device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created  Keyboard device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextKeyboard (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0301);
}

/**
  Converts a text device path node to Serial device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Serial device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextSerial (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0501);
}

/**
  Converts a text device path node to Parallel Port device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Parallel Port device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextParallelPort (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextAcpi (TextDeviceNode, 0x0401);
}

/**
  Converts a text device path node to ACPI extension device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created ACPI extension device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextAcpiEx (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                         *HIDStr;
  CHAR16                         *CIDStr;
  CHAR16                         *UIDStr;
  CHAR16                         *HIDSTRStr;
  CHAR16                         *CIDSTRStr;
  CHAR16                         *UIDSTRStr;
  CHAR8                          *AsciiStr;
  UINT16                         Length;
  ACPI_EXTENDED_HID_DEVICE_PATH  *AcpiEx;

  HIDStr    = GetNextParamStr (&TextDeviceNode);
  CIDStr    = GetNextParamStr (&TextDeviceNode);
  UIDStr    = GetNextParamStr (&TextDeviceNode);
  HIDSTRStr = GetNextParamStr (&TextDeviceNode);
  CIDSTRStr = GetNextParamStr (&TextDeviceNode);
  UIDSTRStr = GetNextParamStr (&TextDeviceNode);

  Length    = (UINT16) (sizeof (ACPI_EXTENDED_HID_DEVICE_PATH) + StrLen (HIDSTRStr) + 1);
  Length    = (UINT16) (Length + StrLen (UIDSTRStr) + 1);
  Length    = (UINT16) (Length + StrLen (CIDSTRStr) + 1);
  AcpiEx = (ACPI_EXTENDED_HID_DEVICE_PATH *) CreateDeviceNode (
                                               ACPI_DEVICE_PATH,
                                               ACPI_EXTENDED_DP,
                                               Length
                                               );

  AcpiEx->HID = EisaIdFromText (HIDStr);
  AcpiEx->CID = EisaIdFromText (CIDStr);
  AcpiEx->UID = (UINT32) Strtoi (UIDStr);

  AsciiStr = (CHAR8 *) ((UINT8 *)AcpiEx + sizeof (ACPI_EXTENDED_HID_DEVICE_PATH));
  StrToAscii (HIDSTRStr, &AsciiStr);
  StrToAscii (UIDSTRStr, &AsciiStr);
  StrToAscii (CIDSTRStr, &AsciiStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) AcpiEx;
}

/**
  Converts a text device path node to ACPI extension device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created ACPI extension device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextAcpiExp (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                         *HIDStr;
  CHAR16                         *CIDStr;
  CHAR16                         *UIDSTRStr;
  CHAR8                          *AsciiStr;
  UINT16                         Length;
  ACPI_EXTENDED_HID_DEVICE_PATH  *AcpiEx;

  HIDStr    = GetNextParamStr (&TextDeviceNode);
  CIDStr    = GetNextParamStr (&TextDeviceNode);
  UIDSTRStr = GetNextParamStr (&TextDeviceNode);
  Length    = (UINT16) (sizeof (ACPI_EXTENDED_HID_DEVICE_PATH) + StrLen (UIDSTRStr) + 3);
  AcpiEx    = (ACPI_EXTENDED_HID_DEVICE_PATH *) CreateDeviceNode (
                                                  ACPI_DEVICE_PATH,
                                                  ACPI_EXTENDED_DP,
                                                  Length
                                                  );

  AcpiEx->HID = EisaIdFromText (HIDStr);
  AcpiEx->CID = EisaIdFromText (CIDStr);
  AcpiEx->UID = 0;

  AsciiStr = (CHAR8 *) ((UINT8 *)AcpiEx + sizeof (ACPI_EXTENDED_HID_DEVICE_PATH));
  //
  // HID string is NULL
  //
  *AsciiStr = '\0';
  //
  // Convert UID string
  //
  AsciiStr++;
  StrToAscii (UIDSTRStr, &AsciiStr);
  //
  // CID string is NULL
  //
  *AsciiStr = '\0';

  return (EFI_DEVICE_PATH_PROTOCOL *) AcpiEx;
}

/**
  Converts a text device path node to ACPI _ADR device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created ACPI _ADR device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextAcpiAdr (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                *DisplayDeviceStr;
  ACPI_ADR_DEVICE_PATH  *AcpiAdr;
  UINTN                 Index;
  UINTN                 Length;

  AcpiAdr = (ACPI_ADR_DEVICE_PATH *) CreateDeviceNode (
                                       ACPI_DEVICE_PATH,
                                       ACPI_ADR_DP,
                                       (UINT16) sizeof (ACPI_ADR_DEVICE_PATH)
                                       );
  ASSERT (AcpiAdr != NULL);

  for (Index = 0; ; Index++) {
    DisplayDeviceStr = GetNextParamStr (&TextDeviceNode);
    if (IS_NULL (*DisplayDeviceStr)) {
      break;
    }
    if (Index > 0) {
      Length  = DevicePathNodeLength (AcpiAdr);
      AcpiAdr = ReallocatePool (
                  Length,
                  Length + sizeof (UINT32),
                  AcpiAdr
                  );
      ASSERT (AcpiAdr != NULL);
      SetDevicePathNodeLength (AcpiAdr, Length + sizeof (UINT32));
    }
    
    (&AcpiAdr->ADR)[Index] = (UINT32) Strtoi (DisplayDeviceStr);
  }

  return (EFI_DEVICE_PATH_PROTOCOL *) AcpiAdr;
}

/**
  Converts a text device path node to Parallel Port device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Parallel Port device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextAta (
IN CHAR16 *TextDeviceNode
)
{
  CHAR16            *PrimarySecondaryStr;
  CHAR16            *SlaveMasterStr;
  CHAR16            *LunStr;
  ATAPI_DEVICE_PATH *Atapi;

  Atapi = (ATAPI_DEVICE_PATH *) CreateDeviceNode (
    MESSAGING_DEVICE_PATH,
    MSG_ATAPI_DP,
    (UINT16) sizeof (ATAPI_DEVICE_PATH)
    );

  PrimarySecondaryStr = GetNextParamStr (&TextDeviceNode);
  SlaveMasterStr      = GetNextParamStr (&TextDeviceNode);
  LunStr              = GetNextParamStr (&TextDeviceNode);

  if (StrCmp (PrimarySecondaryStr, L"Primary") == 0) {
    Atapi->PrimarySecondary = 0;
  } else if (StrCmp (PrimarySecondaryStr, L"Secondary") == 0) {
    Atapi->PrimarySecondary = 1;
  } else {
    Atapi->PrimarySecondary = (UINT8) Strtoi (PrimarySecondaryStr);
  }
  if (StrCmp (SlaveMasterStr, L"Master") == 0) {
    Atapi->SlaveMaster      = 0;
  } else if (StrCmp (SlaveMasterStr, L"Slave") == 0) {
    Atapi->SlaveMaster      = 1;
  } else {
    Atapi->SlaveMaster      = (UINT8) Strtoi (SlaveMasterStr);
  }

  Atapi->Lun                = (UINT16) Strtoi (LunStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Atapi;
}

/**
  Converts a text device path node to SCSI device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created SCSI device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextScsi (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *PunStr;
  CHAR16            *LunStr;
  SCSI_DEVICE_PATH  *Scsi;

  PunStr = GetNextParamStr (&TextDeviceNode);
  LunStr = GetNextParamStr (&TextDeviceNode);
  Scsi   = (SCSI_DEVICE_PATH *) CreateDeviceNode (
                                   MESSAGING_DEVICE_PATH,
                                   MSG_SCSI_DP,
                                   (UINT16) sizeof (SCSI_DEVICE_PATH)
                                   );

  Scsi->Pun = (UINT16) Strtoi (PunStr);
  Scsi->Lun = (UINT16) Strtoi (LunStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Scsi;
}

/**
  Converts a text device path node to Fibre device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Fibre device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFibre (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                    *WWNStr;
  CHAR16                    *LunStr;
  FIBRECHANNEL_DEVICE_PATH  *Fibre;

  WWNStr = GetNextParamStr (&TextDeviceNode);
  LunStr = GetNextParamStr (&TextDeviceNode);
  Fibre  = (FIBRECHANNEL_DEVICE_PATH *) CreateDeviceNode (
                                          MESSAGING_DEVICE_PATH,
                                          MSG_FIBRECHANNEL_DP,
                                          (UINT16) sizeof (FIBRECHANNEL_DEVICE_PATH)
                                          );

  Fibre->Reserved = 0;
  Strtoi64 (WWNStr, &Fibre->WWN);
  Strtoi64 (LunStr, &Fibre->Lun);

  return (EFI_DEVICE_PATH_PROTOCOL *) Fibre;
}

/**
  Converts a text device path node to FibreEx device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created FibreEx device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFibreEx (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                      *WWNStr;
  CHAR16                      *LunStr;
  FIBRECHANNELEX_DEVICE_PATH  *FibreEx;

  WWNStr  = GetNextParamStr (&TextDeviceNode);
  LunStr  = GetNextParamStr (&TextDeviceNode);
  FibreEx = (FIBRECHANNELEX_DEVICE_PATH *) CreateDeviceNode (
                                             MESSAGING_DEVICE_PATH,
                                             MSG_FIBRECHANNELEX_DP,
                                             (UINT16) sizeof (FIBRECHANNELEX_DEVICE_PATH)
                                             );

  FibreEx->Reserved = 0;
  Strtoi64 (WWNStr, (UINT64 *) (&FibreEx->WWN));
  Strtoi64 (LunStr, (UINT64 *) (&FibreEx->Lun));

  *(UINT64 *) (&FibreEx->WWN) = SwapBytes64 (*(UINT64 *) (&FibreEx->WWN));
  *(UINT64 *) (&FibreEx->Lun) = SwapBytes64 (*(UINT64 *) (&FibreEx->Lun));

  return (EFI_DEVICE_PATH_PROTOCOL *) FibreEx;
}

/**
  Converts a text device path node to 1394 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created 1394 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromText1394 (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *GuidStr;
  F1394_DEVICE_PATH *F1394DevPath;

  GuidStr = GetNextParamStr (&TextDeviceNode);
  F1394DevPath  = (F1394_DEVICE_PATH *) CreateDeviceNode (
                                          MESSAGING_DEVICE_PATH,
                                          MSG_1394_DP,
                                          (UINT16) sizeof (F1394_DEVICE_PATH)
                                          );

  F1394DevPath->Reserved = 0;
  F1394DevPath->Guid     = StrHexToUint64 (GuidStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) F1394DevPath;
}

/**
  Converts a text device path node to USB device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsb (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16          *PortStr;
  CHAR16          *InterfaceStr;
  USB_DEVICE_PATH *Usb;

  PortStr               = GetNextParamStr (&TextDeviceNode);
  InterfaceStr          = GetNextParamStr (&TextDeviceNode);
  Usb                   = (USB_DEVICE_PATH *) CreateDeviceNode (
                                                MESSAGING_DEVICE_PATH,
                                                MSG_USB_DP,
                                                (UINT16) sizeof (USB_DEVICE_PATH)
                                                );

  Usb->ParentPortNumber = (UINT8) Strtoi (PortStr);
  Usb->InterfaceNumber  = (UINT8) Strtoi (InterfaceStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Usb;
}

/**
  Converts a text device path node to I20 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created I20 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextI2O (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16          *TIDStr;
  I2O_DEVICE_PATH *I2ODevPath;

  TIDStr     = GetNextParamStr (&TextDeviceNode);
  I2ODevPath = (I2O_DEVICE_PATH *) CreateDeviceNode (
                                    MESSAGING_DEVICE_PATH,
                                    MSG_I2O_DP,
                                    (UINT16) sizeof (I2O_DEVICE_PATH)
                                    );

  I2ODevPath->Tid  = (UINT32) Strtoi (TIDStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) I2ODevPath;
}

/**
  Converts a text device path node to Infini Band device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Infini Band device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextInfiniband (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                  *FlagsStr;
  CHAR16                  *GuidStr;
  CHAR16                  *SidStr;
  CHAR16                  *TidStr;
  CHAR16                  *DidStr;
  EFI_GUID                PortGid;
  INFINIBAND_DEVICE_PATH  *InfiniBand;

  FlagsStr   = GetNextParamStr (&TextDeviceNode);
  GuidStr    = GetNextParamStr (&TextDeviceNode);
  SidStr     = GetNextParamStr (&TextDeviceNode);
  TidStr     = GetNextParamStr (&TextDeviceNode);
  DidStr     = GetNextParamStr (&TextDeviceNode);
  InfiniBand = (INFINIBAND_DEVICE_PATH *) CreateDeviceNode (
                                            MESSAGING_DEVICE_PATH,
                                            MSG_INFINIBAND_DP,
                                            (UINT16) sizeof (INFINIBAND_DEVICE_PATH)
                                            );

  InfiniBand->ResourceFlags = (UINT32) Strtoi (FlagsStr);
  StrToGuid (GuidStr, &PortGid);
  CopyMem (InfiniBand->PortGid, &PortGid, sizeof (EFI_GUID));
  Strtoi64 (SidStr, &InfiniBand->ServiceId);
  Strtoi64 (TidStr, &InfiniBand->TargetPortId);
  Strtoi64 (DidStr, &InfiniBand->DeviceId);

  return (EFI_DEVICE_PATH_PROTOCOL *) InfiniBand;
}

/**
  Converts a text device path node to Vendor-Defined Messaging device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor-Defined Messaging device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenMsg (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextVendor (
            TextDeviceNode,
            MESSAGING_DEVICE_PATH,
            MSG_VENDOR_DP
            );
}

/**
  Converts a text device path node to Vendor defined PC-ANSI device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor defined PC-ANSI device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenPcAnsi (
  IN CHAR16 *TextDeviceNode
  )
{
  VENDOR_DEVICE_PATH  *Vendor;

  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                    MESSAGING_DEVICE_PATH,
                                    MSG_VENDOR_DP,
                                    (UINT16) sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Vendor->Guid, &gEfiPcAnsiGuid);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Vendor defined VT100 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor defined VT100 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenVt100 (
  IN CHAR16 *TextDeviceNode
  )
{
  VENDOR_DEVICE_PATH  *Vendor;

  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                    MESSAGING_DEVICE_PATH,
                                    MSG_VENDOR_DP,
                                    (UINT16) sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Vendor->Guid, &gEfiVT100Guid);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Vendor defined VT100 Plus device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor defined VT100 Plus device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenVt100Plus (
  IN CHAR16 *TextDeviceNode
  )
{
  VENDOR_DEVICE_PATH  *Vendor;

  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                    MESSAGING_DEVICE_PATH,
                                    MSG_VENDOR_DP,
                                    (UINT16) sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Vendor->Guid, &gEfiVT100PlusGuid);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to Vendor defined UTF8 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor defined UTF8 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenUtf8 (
  IN CHAR16 *TextDeviceNode
  )
{
  VENDOR_DEVICE_PATH  *Vendor;

  Vendor = (VENDOR_DEVICE_PATH *) CreateDeviceNode (
                                    MESSAGING_DEVICE_PATH,
                                    MSG_VENDOR_DP,
                                    (UINT16) sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&Vendor->Guid, &gEfiVTUTF8Guid);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vendor;
}

/**
  Converts a text device path node to UART Flow Control device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created UART Flow Control device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUartFlowCtrl (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                        *ValueStr;
  UART_FLOW_CONTROL_DEVICE_PATH *UartFlowControl;

  ValueStr        = GetNextParamStr (&TextDeviceNode);
  UartFlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) CreateDeviceNode (
                                                        MESSAGING_DEVICE_PATH,
                                                        MSG_VENDOR_DP,
                                                        (UINT16) sizeof (UART_FLOW_CONTROL_DEVICE_PATH)
                                                        );

  CopyGuid (&UartFlowControl->Guid, &gEfiUartDevicePathGuid);
  if (StrCmp (ValueStr, L"XonXoff") == 0) {
    UartFlowControl->FlowControlMap = 2;
  } else if (StrCmp (ValueStr, L"Hardware") == 0) {
    UartFlowControl->FlowControlMap = 1;
  } else {
    UartFlowControl->FlowControlMap = 0;
  }

  return (EFI_DEVICE_PATH_PROTOCOL *) UartFlowControl;
}

/**
  Converts a text device path node to Serial Attached SCSI device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Serial Attached SCSI device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextSAS (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16          *AddressStr;
  CHAR16          *LunStr;
  CHAR16          *RTPStr;
  CHAR16          *SASSATAStr;
  CHAR16          *LocationStr;
  CHAR16          *ConnectStr;
  CHAR16          *DriveBayStr;
  CHAR16          *ReservedStr;
  UINT16          Info;
  UINT16          Uint16;
  SAS_DEVICE_PATH *Sas;

  AddressStr  = GetNextParamStr (&TextDeviceNode);
  LunStr      = GetNextParamStr (&TextDeviceNode);
  RTPStr      = GetNextParamStr (&TextDeviceNode);
  SASSATAStr  = GetNextParamStr (&TextDeviceNode);
  LocationStr = GetNextParamStr (&TextDeviceNode);
  ConnectStr  = GetNextParamStr (&TextDeviceNode);
  DriveBayStr = GetNextParamStr (&TextDeviceNode);
  ReservedStr = GetNextParamStr (&TextDeviceNode);
  Sas         = (SAS_DEVICE_PATH *) CreateDeviceNode (
                                       MESSAGING_DEVICE_PATH,
                                       MSG_VENDOR_DP,
                                       (UINT16) sizeof (SAS_DEVICE_PATH)
                                       );

  CopyGuid (&Sas->Guid, &gEfiSasDevicePathGuid);
  Strtoi64 (AddressStr, &Sas->SasAddress);
  Strtoi64 (LunStr, &Sas->Lun);
  Sas->RelativeTargetPort = (UINT16) Strtoi (RTPStr);

  if (StrCmp (SASSATAStr, L"NoTopology") == 0) {
    Info = 0x0;

  } else if ((StrCmp (SASSATAStr, L"SATA") == 0) || (StrCmp (SASSATAStr, L"SAS") == 0)) {

    Uint16 = (UINT16) Strtoi (DriveBayStr);
    if (Uint16 == 0) {
      Info = 0x1;
    } else {
      Info = (UINT16) (0x2 | ((Uint16 - 1) << 8));
    }

    if (StrCmp (SASSATAStr, L"SATA") == 0) {
      Info |= BIT4;
    }

    //
    // Location is an integer between 0 and 1 or else
    // the keyword Internal (0) or External (1).
    //
    if (StrCmp (LocationStr, L"External") == 0) {
      Uint16 = 1;
    } else if (StrCmp (LocationStr, L"Internal") == 0) {
      Uint16 = 0;
    } else {
      Uint16 = ((UINT16) Strtoi (LocationStr) & BIT0);
    }
    Info |= (Uint16 << 5);

    //
    // Connect is an integer between 0 and 3 or else
    // the keyword Direct (0) or Expanded (1).
    //
    if (StrCmp (ConnectStr, L"Expanded") == 0) {
      Uint16 = 1;
    } else if (StrCmp (ConnectStr, L"Direct") == 0) {
      Uint16 = 0;
    } else {
      Uint16 = ((UINT16) Strtoi (ConnectStr) & (BIT0 | BIT1));
    }
    Info |= (Uint16 << 6);

  } else {
    Info = (UINT16) Strtoi (SASSATAStr);
  }

  Sas->DeviceTopology = Info;
  Sas->Reserved       = (UINT32) Strtoi (ReservedStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Sas;
}

/**
  Converts a text device path node to Serial Attached SCSI Ex device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Serial Attached SCSI Ex device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextSasEx (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *AddressStr;
  CHAR16            *LunStr;
  CHAR16            *RTPStr;
  CHAR16            *SASSATAStr;
  CHAR16            *LocationStr;
  CHAR16            *ConnectStr;
  CHAR16            *DriveBayStr;
  UINT16            Info;
  UINT16            Uint16;
  UINT64            SasAddress;
  UINT64            Lun;
  SASEX_DEVICE_PATH *SasEx;

  AddressStr  = GetNextParamStr (&TextDeviceNode);
  LunStr      = GetNextParamStr (&TextDeviceNode);
  RTPStr      = GetNextParamStr (&TextDeviceNode);
  SASSATAStr  = GetNextParamStr (&TextDeviceNode);
  LocationStr = GetNextParamStr (&TextDeviceNode);
  ConnectStr  = GetNextParamStr (&TextDeviceNode);
  DriveBayStr = GetNextParamStr (&TextDeviceNode);
  SasEx       = (SASEX_DEVICE_PATH *) CreateDeviceNode (
                                        MESSAGING_DEVICE_PATH,
                                        MSG_SASEX_DP,
                                        (UINT16) sizeof (SASEX_DEVICE_PATH)
                                        );

  Strtoi64 (AddressStr, &SasAddress);
  Strtoi64 (LunStr,     &Lun);
  WriteUnaligned64 ((UINT64 *) &SasEx->SasAddress, SwapBytes64 (SasAddress));
  WriteUnaligned64 ((UINT64 *) &SasEx->Lun,        SwapBytes64 (Lun));
  SasEx->RelativeTargetPort      = (UINT16) Strtoi (RTPStr);

  if (StrCmp (SASSATAStr, L"NoTopology") == 0) {
    Info = 0x0;

  } else if ((StrCmp (SASSATAStr, L"SATA") == 0) || (StrCmp (SASSATAStr, L"SAS") == 0)) {

    Uint16 = (UINT16) Strtoi (DriveBayStr);
    if (Uint16 == 0) {
      Info = 0x1;
    } else {
      Info = (UINT16) (0x2 | ((Uint16 - 1) << 8));
    }

    if (StrCmp (SASSATAStr, L"SATA") == 0) {
      Info |= BIT4;
    }

    //
    // Location is an integer between 0 and 1 or else
    // the keyword Internal (0) or External (1).
    //
    if (StrCmp (LocationStr, L"External") == 0) {
      Uint16 = 1;
    } else if (StrCmp (LocationStr, L"Internal") == 0) {
      Uint16 = 0;
    } else {
      Uint16 = ((UINT16) Strtoi (LocationStr) & BIT0);
    }
    Info |= (Uint16 << 5);

    //
    // Connect is an integer between 0 and 3 or else
    // the keyword Direct (0) or Expanded (1).
    //
    if (StrCmp (ConnectStr, L"Expanded") == 0) {
      Uint16 = 1;
    } else if (StrCmp (ConnectStr, L"Direct") == 0) {
      Uint16 = 0;
    } else {
      Uint16 = ((UINT16) Strtoi (ConnectStr) & (BIT0 | BIT1));
    }
    Info |= (Uint16 << 6);

  } else {
    Info = (UINT16) Strtoi (SASSATAStr);
  }

  SasEx->DeviceTopology = Info;

  return (EFI_DEVICE_PATH_PROTOCOL *) SasEx;
}

/**
  Converts a text device path node to Debug Port device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Debug Port device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextDebugPort (
  IN CHAR16 *TextDeviceNode
  )
{
  VENDOR_DEFINED_MESSAGING_DEVICE_PATH  *Vend;

  Vend = (VENDOR_DEFINED_MESSAGING_DEVICE_PATH *) CreateDeviceNode (
                                                    MESSAGING_DEVICE_PATH,
                                                    MSG_VENDOR_DP,
                                                    (UINT16) sizeof (VENDOR_DEFINED_MESSAGING_DEVICE_PATH)
                                                    );

  CopyGuid (&Vend->Guid, &gEfiDebugPortProtocolGuid);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vend;
}

/**
  Converts a text device path node to MAC device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created MAC device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextMAC (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                *AddressStr;
  CHAR16                *IfTypeStr;
  UINTN                 Length;
  MAC_ADDR_DEVICE_PATH  *MACDevPath;

  AddressStr    = GetNextParamStr (&TextDeviceNode);
  IfTypeStr     = GetNextParamStr (&TextDeviceNode);
  MACDevPath    = (MAC_ADDR_DEVICE_PATH *) CreateDeviceNode (
                                              MESSAGING_DEVICE_PATH,
                                              MSG_MAC_ADDR_DP,
                                              (UINT16) sizeof (MAC_ADDR_DEVICE_PATH)
                                              );

  MACDevPath->IfType   = (UINT8) Strtoi (IfTypeStr);

  Length = sizeof (EFI_MAC_ADDRESS);
  StrToBuf (&MACDevPath->MacAddress.Addr[0], Length, AddressStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) MACDevPath;
}


/**
  Converts a text format to the network protocol ID.

  @param Text  String of protocol field.

  @return Network protocol ID .

**/
UINTN
NetworkProtocolFromText (
  IN CHAR16 *Text
  )
{
  if (StrCmp (Text, L"UDP") == 0) {
    return RFC_1700_UDP_PROTOCOL;
  }

  if (StrCmp (Text, L"TCP") == 0) {
    return RFC_1700_TCP_PROTOCOL;
  }

  return Strtoi (Text);
}


/**
  Converts a text device path node to IPV4 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created IPV4 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextIPv4 (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *RemoteIPStr;
  CHAR16            *ProtocolStr;
  CHAR16            *TypeStr;
  CHAR16            *LocalIPStr;
  CHAR16            *GatewayIPStr;
  CHAR16            *SubnetMaskStr;
  IPv4_DEVICE_PATH  *IPv4;

  RemoteIPStr           = GetNextParamStr (&TextDeviceNode);
  ProtocolStr           = GetNextParamStr (&TextDeviceNode);
  TypeStr               = GetNextParamStr (&TextDeviceNode);
  LocalIPStr            = GetNextParamStr (&TextDeviceNode);
  GatewayIPStr          = GetNextParamStr (&TextDeviceNode);
  SubnetMaskStr         = GetNextParamStr (&TextDeviceNode);
  IPv4                  = (IPv4_DEVICE_PATH *) CreateDeviceNode (
                                                 MESSAGING_DEVICE_PATH,
                                                 MSG_IPv4_DP,
                                                 (UINT16) sizeof (IPv4_DEVICE_PATH)
                                                 );

  StrToIPv4Addr (&RemoteIPStr, &IPv4->RemoteIpAddress);
  IPv4->Protocol = (UINT16) NetworkProtocolFromText (ProtocolStr);
  if (StrCmp (TypeStr, L"Static") == 0) {
    IPv4->StaticIpAddress = TRUE;
  } else {
    IPv4->StaticIpAddress = FALSE;
  }

  StrToIPv4Addr (&LocalIPStr, &IPv4->LocalIpAddress);
  if (!IS_NULL (*GatewayIPStr) && !IS_NULL (*SubnetMaskStr)) {
    StrToIPv4Addr (&GatewayIPStr,  &IPv4->GatewayIpAddress);
    StrToIPv4Addr (&SubnetMaskStr, &IPv4->SubnetMask);
  } else {
    ZeroMem (&IPv4->GatewayIpAddress, sizeof (IPv4->GatewayIpAddress));
    ZeroMem (&IPv4->SubnetMask,    sizeof (IPv4->SubnetMask));
  }

  IPv4->LocalPort       = 0;
  IPv4->RemotePort      = 0;

  return (EFI_DEVICE_PATH_PROTOCOL *) IPv4;
}

/**
  Converts a text device path node to IPV6 device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created IPV6 device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextIPv6 (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *RemoteIPStr;
  CHAR16            *ProtocolStr;
  CHAR16            *TypeStr;
  CHAR16            *LocalIPStr;
  CHAR16            *GatewayIPStr;
  CHAR16            *PrefixLengthStr;
  IPv6_DEVICE_PATH  *IPv6;

  RemoteIPStr           = GetNextParamStr (&TextDeviceNode);
  ProtocolStr           = GetNextParamStr (&TextDeviceNode);
  TypeStr               = GetNextParamStr (&TextDeviceNode);
  LocalIPStr            = GetNextParamStr (&TextDeviceNode);
  PrefixLengthStr       = GetNextParamStr (&TextDeviceNode);
  GatewayIPStr          = GetNextParamStr (&TextDeviceNode);
  IPv6                  = (IPv6_DEVICE_PATH *) CreateDeviceNode (
                                                 MESSAGING_DEVICE_PATH,
                                                 MSG_IPv6_DP,
                                                 (UINT16) sizeof (IPv6_DEVICE_PATH)
                                                 );

  StrToIPv6Addr (&RemoteIPStr, &IPv6->RemoteIpAddress);
  IPv6->Protocol        = (UINT16) NetworkProtocolFromText (ProtocolStr);
  if (StrCmp (TypeStr, L"Static") == 0) {
    IPv6->IpAddressOrigin = 0;
  } else if (StrCmp (TypeStr, L"StatelessAutoConfigure") == 0) {
    IPv6->IpAddressOrigin = 1;
  } else {
    IPv6->IpAddressOrigin = 2;
  }

  StrToIPv6Addr (&LocalIPStr, &IPv6->LocalIpAddress);
  if (!IS_NULL (*GatewayIPStr) && !IS_NULL (*PrefixLengthStr)) {
    StrToIPv6Addr (&GatewayIPStr, &IPv6->GatewayIpAddress);
    IPv6->PrefixLength = (UINT8) Strtoi (PrefixLengthStr);
  } else {
    ZeroMem (&IPv6->GatewayIpAddress, sizeof (IPv6->GatewayIpAddress));
    IPv6->PrefixLength = 0;
  }

  IPv6->LocalPort       = 0;
  IPv6->RemotePort      = 0;

  return (EFI_DEVICE_PATH_PROTOCOL *) IPv6;
}

/**
  Converts a text device path node to UART device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created UART device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUart (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *BaudStr;
  CHAR16            *DataBitsStr;
  CHAR16            *ParityStr;
  CHAR16            *StopBitsStr;
  UART_DEVICE_PATH  *Uart;

  BaudStr         = GetNextParamStr (&TextDeviceNode);
  DataBitsStr     = GetNextParamStr (&TextDeviceNode);
  ParityStr       = GetNextParamStr (&TextDeviceNode);
  StopBitsStr     = GetNextParamStr (&TextDeviceNode);
  Uart            = (UART_DEVICE_PATH *) CreateDeviceNode (
                                           MESSAGING_DEVICE_PATH,
                                           MSG_UART_DP,
                                           (UINT16) sizeof (UART_DEVICE_PATH)
                                           );

  if (StrCmp (BaudStr, L"DEFAULT") == 0) {
    Uart->BaudRate = 115200;
  } else {
    Strtoi64 (BaudStr, &Uart->BaudRate);
  }
  Uart->DataBits  = (UINT8) ((StrCmp (DataBitsStr, L"DEFAULT") == 0) ? 8 : Strtoi (DataBitsStr));
  switch (*ParityStr) {
  case L'D':
    Uart->Parity = 0;
    break;

  case L'N':
    Uart->Parity = 1;
    break;

  case L'E':
    Uart->Parity = 2;
    break;

  case L'O':
    Uart->Parity = 3;
    break;

  case L'M':
    Uart->Parity = 4;
    break;

  case L'S':
    Uart->Parity = 5;
    break;

  default:
    Uart->Parity = (UINT8) Strtoi (ParityStr);
    break;
  }

  if (StrCmp (StopBitsStr, L"D") == 0) {
    Uart->StopBits = (UINT8) 0;
  } else if (StrCmp (StopBitsStr, L"1") == 0) {
    Uart->StopBits = (UINT8) 1;
  } else if (StrCmp (StopBitsStr, L"1.5") == 0) {
    Uart->StopBits = (UINT8) 2;
  } else if (StrCmp (StopBitsStr, L"2") == 0) {
    Uart->StopBits = (UINT8) 3;
  } else {
    Uart->StopBits = (UINT8) Strtoi (StopBitsStr);
  }

  return (EFI_DEVICE_PATH_PROTOCOL *) Uart;
}

/**
  Converts a text device path node to USB class device path structure.

  @param TextDeviceNode  The input Text device path node.
  @param UsbClassText    A pointer to USB_CLASS_TEXT structure to be integrated to USB Class Text.

  @return A pointer to the newly-created USB class device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
ConvertFromTextUsbClass (
  IN CHAR16         *TextDeviceNode,
  IN USB_CLASS_TEXT *UsbClassText
  )
{
  CHAR16                *VIDStr;
  CHAR16                *PIDStr;
  CHAR16                *ClassStr;
  CHAR16                *SubClassStr;
  CHAR16                *ProtocolStr;
  USB_CLASS_DEVICE_PATH *UsbClass;

  UsbClass    = (USB_CLASS_DEVICE_PATH *) CreateDeviceNode (
                                            MESSAGING_DEVICE_PATH,
                                            MSG_USB_CLASS_DP,
                                            (UINT16) sizeof (USB_CLASS_DEVICE_PATH)
                                            );

  VIDStr      = GetNextParamStr (&TextDeviceNode);
  PIDStr      = GetNextParamStr (&TextDeviceNode);
  if (UsbClassText->ClassExist) {
    ClassStr = GetNextParamStr (&TextDeviceNode);
    UsbClass->DeviceClass = (UINT8) Strtoi (ClassStr);
  } else {
    UsbClass->DeviceClass = UsbClassText->Class;
  }
  if (UsbClassText->SubClassExist) {
    SubClassStr = GetNextParamStr (&TextDeviceNode);
    UsbClass->DeviceSubClass = (UINT8) Strtoi (SubClassStr);
  } else {
    UsbClass->DeviceSubClass = UsbClassText->SubClass;
  }

  ProtocolStr = GetNextParamStr (&TextDeviceNode);

  UsbClass->VendorId        = (UINT16) Strtoi (VIDStr);
  UsbClass->ProductId       = (UINT16) Strtoi (PIDStr);
  UsbClass->DeviceProtocol  = (UINT8) Strtoi (ProtocolStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) UsbClass;
}


/**
  Converts a text device path node to USB class device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB class device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbClass (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = TRUE;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB audio device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB audio device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbAudio (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_AUDIO;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB CDC Control device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB CDC Control device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbCDCControl (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_CDCCONTROL;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB HID device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB HID device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbHID (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_HID;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB Image device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB Image device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbImage (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_IMAGE;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB Print device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB Print device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbPrinter (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_PRINTER;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB mass storage device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB mass storage device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbMassStorage (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_MASS_STORAGE;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB HUB device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB HUB device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbHub (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_HUB;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB CDC data device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB CDC data device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbCDCData (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_CDCDATA;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB smart card device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB smart card device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbSmartCard (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_SMART_CARD;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB video device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB video device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbVideo (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_VIDEO;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB diagnostic device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB diagnostic device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbDiagnostic (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_DIAGNOSTIC;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB wireless device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB wireless device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbWireless (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_WIRELESS;
  UsbClassText.SubClassExist = TRUE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB device firmware update device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB device firmware update device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbDeviceFirmwareUpdate (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_RESERVE;
  UsbClassText.SubClassExist = FALSE;
  UsbClassText.SubClass      = USB_SUBCLASS_FW_UPDATE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB IRDA bridge device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB IRDA bridge device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbIrdaBridge (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_RESERVE;
  UsbClassText.SubClassExist = FALSE;
  UsbClassText.SubClass      = USB_SUBCLASS_IRDA_BRIDGE;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB text and measurement device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB text and measurement device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbTestAndMeasurement (
  IN CHAR16 *TextDeviceNode
  )
{
  USB_CLASS_TEXT  UsbClassText;

  UsbClassText.ClassExist    = FALSE;
  UsbClassText.Class         = USB_CLASS_RESERVE;
  UsbClassText.SubClassExist = FALSE;
  UsbClassText.SubClass      = USB_SUBCLASS_TEST;

  return ConvertFromTextUsbClass (TextDeviceNode, &UsbClassText);
}

/**
  Converts a text device path node to USB WWID device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created USB WWID device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUsbWwid (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                *VIDStr;
  CHAR16                *PIDStr;
  CHAR16                *InterfaceNumStr;
  CHAR16                *SerialNumberStr;
  USB_WWID_DEVICE_PATH  *UsbWwid;
  UINTN                 SerialNumberStrLen;

  VIDStr                   = GetNextParamStr (&TextDeviceNode);
  PIDStr                   = GetNextParamStr (&TextDeviceNode);
  InterfaceNumStr          = GetNextParamStr (&TextDeviceNode);
  SerialNumberStr          = GetNextParamStr (&TextDeviceNode);
  SerialNumberStrLen       = StrLen (SerialNumberStr);
  if (SerialNumberStrLen >= 2 &&
      SerialNumberStr[0] == L'\"' &&
      SerialNumberStr[SerialNumberStrLen - 1] == L'\"'
    ) {
    SerialNumberStr[SerialNumberStrLen - 1] = L'\0';
    SerialNumberStr++;
    SerialNumberStrLen -= 2;
  }
  UsbWwid                  = (USB_WWID_DEVICE_PATH *) CreateDeviceNode (
                                                         MESSAGING_DEVICE_PATH,
                                                         MSG_USB_WWID_DP,
                                                         (UINT16) (sizeof (USB_WWID_DEVICE_PATH) + SerialNumberStrLen * sizeof (CHAR16))
                                                         );
  UsbWwid->VendorId        = (UINT16) Strtoi (VIDStr);
  UsbWwid->ProductId       = (UINT16) Strtoi (PIDStr);
  UsbWwid->InterfaceNumber = (UINT16) Strtoi (InterfaceNumStr);
  StrnCpy ((CHAR16 *) ((UINT8 *) UsbWwid + sizeof (USB_WWID_DEVICE_PATH)), SerialNumberStr, SerialNumberStrLen);

  return (EFI_DEVICE_PATH_PROTOCOL *) UsbWwid;
}

/**
  Converts a text device path node to Logic Unit device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Logic Unit device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextUnit (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                          *LunStr;
  DEVICE_LOGICAL_UNIT_DEVICE_PATH *LogicalUnit;

  LunStr      = GetNextParamStr (&TextDeviceNode);
  LogicalUnit = (DEVICE_LOGICAL_UNIT_DEVICE_PATH *) CreateDeviceNode (
                                                      MESSAGING_DEVICE_PATH,
                                                      MSG_DEVICE_LOGICAL_UNIT_DP,
                                                      (UINT16) sizeof (DEVICE_LOGICAL_UNIT_DEVICE_PATH)
                                                      );

  LogicalUnit->Lun  = (UINT8) Strtoi (LunStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) LogicalUnit;
}

/**
  Converts a text device path node to iSCSI device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created iSCSI device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextiSCSI (
  IN CHAR16 *TextDeviceNode
  )
{
  UINT16                      Options;
  CHAR16                      *NameStr;
  CHAR16                      *PortalGroupStr;
  CHAR16                      *LunStr;
  CHAR16                      *HeaderDigestStr;
  CHAR16                      *DataDigestStr;
  CHAR16                      *AuthenticationStr;
  CHAR16                      *ProtocolStr;
  CHAR8                       *AsciiStr;
  ISCSI_DEVICE_PATH_WITH_NAME *ISCSIDevPath;

  NameStr           = GetNextParamStr (&TextDeviceNode);
  PortalGroupStr    = GetNextParamStr (&TextDeviceNode);
  LunStr            = GetNextParamStr (&TextDeviceNode);
  HeaderDigestStr   = GetNextParamStr (&TextDeviceNode);
  DataDigestStr     = GetNextParamStr (&TextDeviceNode);
  AuthenticationStr = GetNextParamStr (&TextDeviceNode);
  ProtocolStr       = GetNextParamStr (&TextDeviceNode);
  ISCSIDevPath      = (ISCSI_DEVICE_PATH_WITH_NAME *) CreateDeviceNode (
                                                        MESSAGING_DEVICE_PATH,
                                                        MSG_ISCSI_DP,
                                                        (UINT16) (sizeof (ISCSI_DEVICE_PATH_WITH_NAME) + StrLen (NameStr))
                                                        );

  AsciiStr = ISCSIDevPath->TargetName;
  StrToAscii (NameStr, &AsciiStr);

  ISCSIDevPath->TargetPortalGroupTag = (UINT16) Strtoi (PortalGroupStr);
  Strtoi64 (LunStr, &ISCSIDevPath->Lun);

  Options = 0x0000;
  if (StrCmp (HeaderDigestStr, L"CRC32C") == 0) {
    Options |= 0x0002;
  }

  if (StrCmp (DataDigestStr, L"CRC32C") == 0) {
    Options |= 0x0008;
  }

  if (StrCmp (AuthenticationStr, L"None") == 0) {
    Options |= 0x0800;
  }

  if (StrCmp (AuthenticationStr, L"CHAP_UNI") == 0) {
    Options |= 0x1000;
  }

  ISCSIDevPath->LoginOption      = (UINT16) Options;

  ISCSIDevPath->NetworkProtocol  = (UINT16) StrCmp (ProtocolStr, L"TCP");

  return (EFI_DEVICE_PATH_PROTOCOL *) ISCSIDevPath;
}

/**
  Converts a text device path node to VLAN device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created VLAN device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVlan (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *VlanStr;
  VLAN_DEVICE_PATH  *Vlan;

  VlanStr = GetNextParamStr (&TextDeviceNode);
  Vlan    = (VLAN_DEVICE_PATH *) CreateDeviceNode (
                                   MESSAGING_DEVICE_PATH,
                                   MSG_VLAN_DP,
                                   (UINT16) sizeof (VLAN_DEVICE_PATH)
                                   );

  Vlan->VlanId = (UINT16) Strtoi (VlanStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Vlan;
}

/**
  Converts a text device path node to HD device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created HD device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextHD (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                *PartitionStr;
  CHAR16                *TypeStr;
  CHAR16                *SignatureStr;
  CHAR16                *StartStr;
  CHAR16                *SizeStr;
  UINT32                Signature32;
  EFI_GUID              SignatureGuid;
  HARDDRIVE_DEVICE_PATH *Hd;

  PartitionStr        = GetNextParamStr (&TextDeviceNode);
  TypeStr             = GetNextParamStr (&TextDeviceNode);
  SignatureStr        = GetNextParamStr (&TextDeviceNode);
  StartStr            = GetNextParamStr (&TextDeviceNode);
  SizeStr             = GetNextParamStr (&TextDeviceNode);
  Hd                  = (HARDDRIVE_DEVICE_PATH *) CreateDeviceNode (
                                                    MEDIA_DEVICE_PATH,
                                                    MEDIA_HARDDRIVE_DP,
                                                    (UINT16) sizeof (HARDDRIVE_DEVICE_PATH)
                                                    );

  Hd->PartitionNumber = (UINT32) Strtoi (PartitionStr);

  ZeroMem (Hd->Signature, 16);
  Hd->MBRType = (UINT8) 0;

  if (StrCmp (TypeStr, L"MBR") == 0) {
    Hd->SignatureType = SIGNATURE_TYPE_MBR;
    Hd->MBRType       = 0x01;

    Signature32       = (UINT32) Strtoi (SignatureStr);
    CopyMem (Hd->Signature, &Signature32, sizeof (UINT32));
  } else if (StrCmp (TypeStr, L"GPT") == 0) {
    Hd->SignatureType = SIGNATURE_TYPE_GUID;
    Hd->MBRType       = 0x02;

    StrToGuid (SignatureStr, &SignatureGuid);
    CopyMem (Hd->Signature, &SignatureGuid, sizeof (EFI_GUID));
  } else {
    Hd->SignatureType = (UINT8) Strtoi (TypeStr);
  }

  Strtoi64 (StartStr, &Hd->PartitionStart);
  Strtoi64 (SizeStr, &Hd->PartitionSize);

  return (EFI_DEVICE_PATH_PROTOCOL *) Hd;
}

/**
  Converts a text device path node to CDROM device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created CDROM device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextCDROM (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16            *EntryStr;
  CHAR16            *StartStr;
  CHAR16            *SizeStr;
  CDROM_DEVICE_PATH *CDROMDevPath;

  EntryStr              = GetNextParamStr (&TextDeviceNode);
  StartStr              = GetNextParamStr (&TextDeviceNode);
  SizeStr               = GetNextParamStr (&TextDeviceNode);
  CDROMDevPath          = (CDROM_DEVICE_PATH *) CreateDeviceNode (
                                                  MEDIA_DEVICE_PATH,
                                                  MEDIA_CDROM_DP,
                                                  (UINT16) sizeof (CDROM_DEVICE_PATH)
                                                  );

  CDROMDevPath->BootEntry = (UINT32) Strtoi (EntryStr);
  Strtoi64 (StartStr, &CDROMDevPath->PartitionStart);
  Strtoi64 (SizeStr, &CDROMDevPath->PartitionSize);

  return (EFI_DEVICE_PATH_PROTOCOL *) CDROMDevPath;
}

/**
  Converts a text device path node to Vendor-defined media device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Vendor-defined media device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextVenMedia (
  IN CHAR16 *TextDeviceNode
  )
{
  return ConvertFromTextVendor (
           TextDeviceNode,
           MEDIA_DEVICE_PATH,
           MEDIA_VENDOR_DP
           );
}

/**
  Converts a text device path node to File device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created File device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFilePath (
  IN CHAR16 *TextDeviceNode
  )
{
  FILEPATH_DEVICE_PATH  *File;

  File = (FILEPATH_DEVICE_PATH *) CreateDeviceNode (
                                    MEDIA_DEVICE_PATH,
                                    MEDIA_FILEPATH_DP,
                                    (UINT16) (sizeof (FILEPATH_DEVICE_PATH) + StrLen (TextDeviceNode) * 2)
                                    );

  StrCpy (File->PathName, TextDeviceNode);

  return (EFI_DEVICE_PATH_PROTOCOL *) File;
}

/**
  Converts a text device path node to Media protocol device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Media protocol device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextMedia (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                      *GuidStr;
  MEDIA_PROTOCOL_DEVICE_PATH  *Media;

  GuidStr = GetNextParamStr (&TextDeviceNode);
  Media   = (MEDIA_PROTOCOL_DEVICE_PATH *) CreateDeviceNode (
                                             MEDIA_DEVICE_PATH,
                                             MEDIA_PROTOCOL_DP,
                                             (UINT16) sizeof (MEDIA_PROTOCOL_DEVICE_PATH)
                                             );

  StrToGuid (GuidStr, &Media->Protocol);

  return (EFI_DEVICE_PATH_PROTOCOL *) Media;
}

/**
  Converts a text device path node to firmware volume device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created firmware volume device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFv (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                    *GuidStr;
  MEDIA_FW_VOL_DEVICE_PATH  *Fv;

  GuidStr = GetNextParamStr (&TextDeviceNode);
  Fv      = (MEDIA_FW_VOL_DEVICE_PATH *) CreateDeviceNode (
                                           MEDIA_DEVICE_PATH,
                                           MEDIA_PIWG_FW_VOL_DP,
                                           (UINT16) sizeof (MEDIA_FW_VOL_DEVICE_PATH)
                                           );

  StrToGuid (GuidStr, &Fv->FvName);

  return (EFI_DEVICE_PATH_PROTOCOL *) Fv;
}

/**
  Converts a text device path node to firmware file device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created firmware file device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextFvFile (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                             *GuidStr;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFile;

  GuidStr = GetNextParamStr (&TextDeviceNode);
  FvFile  = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) CreateDeviceNode (
                                                    MEDIA_DEVICE_PATH,
                                                    MEDIA_PIWG_FW_FILE_DP,
                                                    (UINT16) sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH)
                                                    );

  StrToGuid (GuidStr, &FvFile->FvFileName);

  return (EFI_DEVICE_PATH_PROTOCOL *) FvFile;
}

/**
  Converts a text device path node to text relative offset device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created Text device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextRelativeOffsetRange (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16                                  *StartingOffsetStr;
  CHAR16                                  *EndingOffsetStr;
  MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH *Offset;

  StartingOffsetStr = GetNextParamStr (&TextDeviceNode);
  EndingOffsetStr   = GetNextParamStr (&TextDeviceNode);
  Offset            = (MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH *) CreateDeviceNode (
                                                                    MEDIA_DEVICE_PATH,
                                                                    MEDIA_RELATIVE_OFFSET_RANGE_DP,
                                                                    (UINT16) sizeof (MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH)
                                                                    );

  Strtoi64 (StartingOffsetStr, &Offset->StartingOffset);
  Strtoi64 (EndingOffsetStr, &Offset->EndingOffset);

  return (EFI_DEVICE_PATH_PROTOCOL *) Offset;
}

/**
  Converts a text device path node to BIOS Boot Specification device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created BIOS Boot Specification device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextBBS (
  IN CHAR16 *TextDeviceNode
  )
{
  CHAR16              *TypeStr;
  CHAR16              *IdStr;
  CHAR16              *FlagsStr;
  CHAR8               *AsciiStr;
  BBS_BBS_DEVICE_PATH *Bbs;

  TypeStr   = GetNextParamStr (&TextDeviceNode);
  IdStr     = GetNextParamStr (&TextDeviceNode);
  FlagsStr  = GetNextParamStr (&TextDeviceNode);
  Bbs       = (BBS_BBS_DEVICE_PATH *) CreateDeviceNode (
                                        BBS_DEVICE_PATH,
                                        BBS_BBS_DP,
                                        (UINT16) (sizeof (BBS_BBS_DEVICE_PATH) + StrLen (IdStr))
                                        );

  if (StrCmp (TypeStr, L"Floppy") == 0) {
    Bbs->DeviceType = BBS_TYPE_FLOPPY;
  } else if (StrCmp (TypeStr, L"HD") == 0) {
    Bbs->DeviceType = BBS_TYPE_HARDDRIVE;
  } else if (StrCmp (TypeStr, L"CDROM") == 0) {
    Bbs->DeviceType = BBS_TYPE_CDROM;
  } else if (StrCmp (TypeStr, L"PCMCIA") == 0) {
    Bbs->DeviceType = BBS_TYPE_PCMCIA;
  } else if (StrCmp (TypeStr, L"USB") == 0) {
    Bbs->DeviceType = BBS_TYPE_USB;
  } else if (StrCmp (TypeStr, L"Network") == 0) {
    Bbs->DeviceType = BBS_TYPE_EMBEDDED_NETWORK;
  } else {
    Bbs->DeviceType = (UINT16) Strtoi (TypeStr);
  }

  AsciiStr = Bbs->String;
  StrToAscii (IdStr, &AsciiStr);

  Bbs->StatusFlag = (UINT16) Strtoi (FlagsStr);

  return (EFI_DEVICE_PATH_PROTOCOL *) Bbs;
}

/**
  Converts a text device path node to SATA device path structure.

  @param TextDeviceNode  The input Text device path node.

  @return A pointer to the newly-created SATA device path structure.

**/
EFI_DEVICE_PATH_PROTOCOL *
DevPathFromTextSata (
  IN CHAR16 *TextDeviceNode
  )
{
  SATA_DEVICE_PATH *Sata;
  CHAR16           *Param1;
  CHAR16           *Param2;
  CHAR16           *Param3;

  Param1 = GetNextParamStr (&TextDeviceNode);
  Param2 = GetNextParamStr (&TextDeviceNode);
  Param3 = GetNextParamStr (&TextDeviceNode);

  Sata = (SATA_DEVICE_PATH *) CreateDeviceNode (
                                MESSAGING_DEVICE_PATH,
                                MSG_SATA_DP,
                                (UINT16) sizeof (SATA_DEVICE_PATH)
                                );
  Sata->HBAPortNumber            = (UINT16) Strtoi (Param1);
  Sata->PortMultiplierPortNumber = (UINT16) Strtoi (Param2);
  Sata->Lun                      = (UINT16) Strtoi (Param3);

  return (EFI_DEVICE_PATH_PROTOCOL *) Sata;
}

GLOBAL_REMOVE_IF_UNREFERENCED DEVICE_PATH_FROM_TEXT_TABLE mUefiDevicePathLibDevPathFromTextTable[] = {
  {L"Pci",                     DevPathFromTextPci                     },
  {L"PcCard",                  DevPathFromTextPcCard                  },
  {L"MemoryMapped",            DevPathFromTextMemoryMapped            },
  {L"VenHw",                   DevPathFromTextVenHw                   },
  {L"Ctrl",                    DevPathFromTextCtrl                    },
  {L"Acpi",                    DevPathFromTextAcpi                    },
  {L"PciRoot",                 DevPathFromTextPciRoot                 },
  {L"PcieRoot",                DevPathFromTextPcieRoot                },
  {L"Floppy",                  DevPathFromTextFloppy                  },
  {L"Keyboard",                DevPathFromTextKeyboard                },
  {L"Serial",                  DevPathFromTextSerial                  },
  {L"ParallelPort",            DevPathFromTextParallelPort            },
  {L"AcpiEx",                  DevPathFromTextAcpiEx                  },
  {L"AcpiExp",                 DevPathFromTextAcpiExp                 },
  {L"AcpiAdr",                 DevPathFromTextAcpiAdr                 },
  {L"Ata",                     DevPathFromTextAta                     },
  {L"Scsi",                    DevPathFromTextScsi                    },
  {L"Fibre",                   DevPathFromTextFibre                   },
  {L"FibreEx",                 DevPathFromTextFibreEx                 },
  {L"I1394",                   DevPathFromText1394                    },
  {L"USB",                     DevPathFromTextUsb                     },
  {L"I2O",                     DevPathFromTextI2O                     },
  {L"Infiniband",              DevPathFromTextInfiniband              },
  {L"VenMsg",                  DevPathFromTextVenMsg                  },
  {L"VenPcAnsi",               DevPathFromTextVenPcAnsi               },
  {L"VenVt100",                DevPathFromTextVenVt100                },
  {L"VenVt100Plus",            DevPathFromTextVenVt100Plus            },
  {L"VenUtf8",                 DevPathFromTextVenUtf8                 },
  {L"UartFlowCtrl",            DevPathFromTextUartFlowCtrl            },
  {L"SAS",                     DevPathFromTextSAS                     },
  {L"SasEx",                   DevPathFromTextSasEx                   },
  {L"DebugPort",               DevPathFromTextDebugPort               },
  {L"MAC",                     DevPathFromTextMAC                     },
  {L"IPv4",                    DevPathFromTextIPv4                    },
  {L"IPv6",                    DevPathFromTextIPv6                    },
  {L"Uart",                    DevPathFromTextUart                    },
  {L"UsbClass",                DevPathFromTextUsbClass                },
  {L"UsbAudio",                DevPathFromTextUsbAudio                },
  {L"UsbCDCControl",           DevPathFromTextUsbCDCControl           },
  {L"UsbHID",                  DevPathFromTextUsbHID                  },
  {L"UsbImage",                DevPathFromTextUsbImage                },
  {L"UsbPrinter",              DevPathFromTextUsbPrinter              },
  {L"UsbMassStorage",          DevPathFromTextUsbMassStorage          },
  {L"UsbHub",                  DevPathFromTextUsbHub                  },
  {L"UsbCDCData",              DevPathFromTextUsbCDCData              },
  {L"UsbSmartCard",            DevPathFromTextUsbSmartCard            },
  {L"UsbVideo",                DevPathFromTextUsbVideo                },
  {L"UsbDiagnostic",           DevPathFromTextUsbDiagnostic           },
  {L"UsbWireless",             DevPathFromTextUsbWireless             },
  {L"UsbDeviceFirmwareUpdate", DevPathFromTextUsbDeviceFirmwareUpdate },
  {L"UsbIrdaBridge",           DevPathFromTextUsbIrdaBridge           },
  {L"UsbTestAndMeasurement",   DevPathFromTextUsbTestAndMeasurement   },
  {L"UsbWwid",                 DevPathFromTextUsbWwid                 },
  {L"Unit",                    DevPathFromTextUnit                    },
  {L"iSCSI",                   DevPathFromTextiSCSI                   },
  {L"Vlan",                    DevPathFromTextVlan                    },
  {L"HD",                      DevPathFromTextHD                      },
  {L"CDROM",                   DevPathFromTextCDROM                   },
  {L"VenMedia",                DevPathFromTextVenMedia                },
  {L"Media",                   DevPathFromTextMedia                   },
  {L"Fv",                      DevPathFromTextFv                      },
  {L"FvFile",                  DevPathFromTextFvFile                  },
  {L"Offset",                  DevPathFromTextRelativeOffsetRange     },
  {L"BBS",                     DevPathFromTextBBS                     },
  {L"Sata",                    DevPathFromTextSata                    },
  {NULL, NULL}
};

/**
  Convert text to the binary representation of a device node.

  @param TextDeviceNode  TextDeviceNode points to the text representation of a device
                         node. Conversion starts with the first character and continues
                         until the first non-device node character.

  @return A pointer to the EFI device node or NULL if TextDeviceNode is NULL or there was
          insufficient memory or text unsupported.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
UefiDevicePathLibConvertTextToDeviceNode (
  IN CONST CHAR16 *TextDeviceNode
  )
{
  DEVICE_PATH_FROM_TEXT    FromText;
  CHAR16                   *ParamStr;
  EFI_DEVICE_PATH_PROTOCOL *DeviceNode;
  CHAR16                   *DeviceNodeStr;
  UINTN                    Index;

  if ((TextDeviceNode == NULL) || (IS_NULL (*TextDeviceNode))) {
    return NULL;
  }

  ParamStr      = NULL;
  FromText      = NULL;
  DeviceNodeStr = UefiDevicePathLibStrDuplicate (TextDeviceNode);
  ASSERT (DeviceNodeStr != NULL);

  for (Index = 0; mUefiDevicePathLibDevPathFromTextTable[Index].Function != NULL; Index++) {
    ParamStr = GetParamByNodeName (DeviceNodeStr, mUefiDevicePathLibDevPathFromTextTable[Index].DevicePathNodeText);
    if (ParamStr != NULL) {
      FromText = mUefiDevicePathLibDevPathFromTextTable[Index].Function;
      break;
    }
  }

  if (FromText == NULL) {
    //
    // A file path
    //
    FromText = DevPathFromTextFilePath;
    DeviceNode = FromText (DeviceNodeStr);
  } else {
    DeviceNode = FromText (ParamStr);
    FreePool (ParamStr);
  }

  FreePool (DeviceNodeStr);

  return DeviceNode;
}

/**
  Convert text to the binary representation of a device path.


  @param TextDevicePath  TextDevicePath points to the text representation of a device
                         path. Conversion starts with the first character and continues
                         until the first non-device node character.

  @return A pointer to the allocated device path or NULL if TextDeviceNode is NULL or
          there was insufficient memory.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
UefiDevicePathLibConvertTextToDevicePath (
  IN CONST CHAR16 *TextDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL *DeviceNode;
  EFI_DEVICE_PATH_PROTOCOL *NewDevicePath;
  CHAR16                   *DevicePathStr;
  CHAR16                   *Str;
  CHAR16                   *DeviceNodeStr;
  BOOLEAN                  IsInstanceEnd;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  if ((TextDevicePath == NULL) || (IS_NULL (*TextDevicePath))) {
    return NULL;
  }

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (END_DEVICE_PATH_LENGTH);
  ASSERT (DevicePath != NULL);
  SetDevicePathEndNode (DevicePath);

  DevicePathStr = UefiDevicePathLibStrDuplicate (TextDevicePath);

  Str           = DevicePathStr;
  while ((DeviceNodeStr = GetNextDeviceNodeStr (&Str, &IsInstanceEnd)) != NULL) {
    DeviceNode = UefiDevicePathLibConvertTextToDeviceNode (DeviceNodeStr);

    NewDevicePath = AppendDevicePathNode (DevicePath, DeviceNode);
    FreePool (DevicePath);
    FreePool (DeviceNode);
    DevicePath = NewDevicePath;

    if (IsInstanceEnd) {
      DeviceNode = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (END_DEVICE_PATH_LENGTH);
      ASSERT (DeviceNode != NULL);
      SetDevicePathEndNode (DeviceNode);

      NewDevicePath = AppendDevicePathNode (DevicePath, DeviceNode);
      FreePool (DevicePath);
      FreePool (DeviceNode);
      DevicePath = NewDevicePath;
    }
  }

  FreePool (DevicePathStr);
  return DevicePath;
}
