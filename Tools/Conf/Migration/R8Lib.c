/** @file
  Obsolete library interfaces.

  This file contains part of obsolete library interfaces in EDK.
  User is recommended to follow the porting Guide in R8Lib.c to elimante them.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

////
BOOLEAN
R8_EfiLibCompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
/*++

Routine Description:

  Compare whether two names of languages are identical.

Arguments:

  Language1 - Name of language 1
  Language2 - Name of language 2

Returns:

  TRUE      - same
  FALSE     - not same

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //
  UINTN Index;

  for (Index = 0; Index < 3; Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  return TRUE;
}
////~

////#BaseLib
EFI_STATUS
R8_BufToHexString (
  IN OUT CHAR16                    *Str,
  IN OUT UINTN                     *HexStringBufferLength,
  IN     UINT8                     *Buf,
  IN     UINTN                      Len
  )
/*++

  Routine Description:
    Converts binary buffer to Unicode string.
    At a minimum, any blob of data could be represented as a hex string.

  Arguments:
    Str                   - Pointer to the string.
    HexStringBufferLength - Length in bytes of buffer to hold the hex string. Includes tailing '\0' character.
                            If routine return with EFI_SUCCESS, containing length of hex string buffer.
                            If routine return with EFI_BUFFER_TOO_SMALL, containg length of hex string buffer desired.
    Buf                   - Buffer to be converted from.
    Len                   - Length in bytes of the buffer to be converted.

  Returns:
    EFI_SUCCESS             - Routine success.
    EFI_BUFFER_TOO_SMALL    - The hex string buffer is too small.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //
  UINTN       Idx;
  UINT8       Byte;
  UINTN       StrLen;

  //
  // Make sure string is either passed or allocate enough.
  // It takes 2 Unicode characters (4 bytes) to represent 1 byte of the binary buffer.
  // Plus the Unicode termination character.
  //
  StrLen = Len * 2;
  if (StrLen > ((*HexStringBufferLength) - 1)) {
    *HexStringBufferLength = StrLen + 1;
    return EFI_BUFFER_TOO_SMALL;
  }

  *HexStringBufferLength = StrLen + 1;
  //
  // Ends the string.
  //
  Str[StrLen] = L'\0'; 

  for (Idx = 0; Idx < Len; Idx++) {

    Byte = Buf[Idx];
    Str[StrLen - 1 - Idx * 2] = NibbleToHexChar (Byte);
    Str[StrLen - 2 - Idx * 2] = NibbleToHexChar ((UINT8)(Byte >> 4));
  }

  return EFI_SUCCESS;
}
////~

////
VOID
R8_EfiStrTrim (
  IN OUT CHAR16   *str,
  IN     CHAR16   CharC
  )
/*++

Routine Description:
  
  Removes (trims) specified leading and trailing characters from a string.
  
Arguments: 
  
  str     - Pointer to the null-terminated string to be trimmed. On return, 
            str will hold the trimmed string. 
  CharC   - Character will be trimmed from str.
  
--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //
  CHAR16  *p1;
  CHAR16  *p2;
  
  if (*str == 0) {
    return;
  }
  
  //
  // Trim off the leading and trailing characters c
  //
  for (p1 = str; *p1 && *p1 == CharC; p1++) {
    ;
  }
  
  p2 = str;
  if (p2 == p1) {
    while (*p1) {
      p2++;
      p1++;
    }
  } else {
    while (*p1) {    
    *p2 = *p1;    
    p1++;
    p2++;
    }
    *p2 = 0;
  }
  
  
  for (p1 = str + StrLen(str) - 1; p1 >= str && *p1 == CharC; p1--) {
    ;
  }
  if  (p1 !=  str + StrLen(str) - 1) { 
    *(p1 + 1) = 0;
  }
}
////~

////#PrintLib
UINTN
R8_EfiValueToHexStr (
  IN  OUT CHAR16  *Buffer, 
  IN  UINT64      Value, 
  IN  UINTN       Flags, 
  IN  UINTN       Width
  )
/*++

Routine Description:

  VSPrint worker function that prints a Value as a hex number in Buffer

Arguments:

  Buffer - Location to place ascii hex string of Value.
  Value  - Hex value to convert to a string in Buffer.
  Flags  - Flags to use in printing Hex string, see file header for details.
  Width  - Width of hex value.

Returns: 

  Number of characters printed.  

--*/
{
  //
  // Porting Guide:
  // Edk II BasePrintLib function UnicodeValueToString does not support 
  // to convert Value to Hex String. 
  // Include the source code to user code or use the full PrintLib funtion 
  // UnicodeVSPrintAsciiFormat (Buffer, MAXIMUM_VALUE_CHARACTERS, "%x", Value) instead.
  //

  CHAR16  TempBuffer[MAXIMUM_VALUE_CHARACTERS];
  CHAR16  *TempStr;
  CHAR16  Prefix;
  CHAR16  *BufferPtr;
  UINTN   Count;
  UINTN   Index;

  TempStr = TempBuffer;
  BufferPtr = Buffer;

  //
  // Count starts at one since we will null terminate. Each iteration of the
  // loop picks off one nibble. Oh yea TempStr ends up backwards
  //
  Count = 0;
  
  if (Width > MAXIMUM_VALUE_CHARACTERS - 1) {
    Width = MAXIMUM_VALUE_CHARACTERS - 1;
  }

  do {
    //
    // If Width == 0, it means no limit.
    //
    if ((Width != 0) && (Count >= Width)) {
      break;
    }

    Index = ((UINTN)Value & 0xf);
    *(TempStr++) = mHexStr[Index];
    Value = RShiftU64 (Value, 4);
    Count++;
  } while (Value != 0);

  if (Flags & PREFIX_ZERO) {
    Prefix = '0';
  } else { 
    Prefix = ' ';
  }

  Index = Count;
  if (!(Flags & LEFT_JUSTIFY)) {
    for (; Index < Width; Index++) {
      *(TempStr++) = Prefix;
    }
  }

  //
  // Reverse temp string into Buffer.
  //
  while (TempStr != TempBuffer) {
    *(BufferPtr++) = *(--TempStr);
  }  
    
  *BufferPtr = 0;
  return Index;
}
////~



////
EFI_STATUS
R8_HexStringToBuf (
  IN OUT UINT8                     *Buf,   
  IN OUT UINTN                    *Len,
  IN     CHAR16                    *Str,
  OUT    UINTN                     *ConvertedStrLen  OPTIONAL
  )
/*++

  Routine Description:
    Converts Unicode string to binary buffer.
    The conversion may be partial.
    The first character in the string that is not hex digit stops the conversion.
    At a minimum, any blob of data could be represented as a hex string.

  Arguments:
    Buf    - Pointer to buffer that receives the data.
    Len    - Length in bytes of the buffer to hold converted data.
                If routine return with EFI_SUCCESS, containing length of converted data.
                If routine return with EFI_BUFFER_TOO_SMALL, containg length of buffer desired.
    Str    - String to be converted from.
    ConvertedStrLen - Length of the Hex String consumed.

  Returns:
    EFI_SUCCESS           - Routine Success.
    EFI_BUFFER_TOO_SMALL  - The buffer is too small to hold converted data.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //

  UINTN       HexCnt;
  UINTN       Idx;
  UINTN       BufferLength;
  UINT8       Digit;
  UINT8       Byte;

  //
  // Find out how many hex characters the string has.
  //
  for (Idx = 0, HexCnt = 0; R8_IsHexDigit (&Digit, Str[Idx]); Idx++, HexCnt++);

  if (HexCnt == 0) {
    *Len = 0;
    return EFI_SUCCESS;
  }
  //
  // Two Unicode characters make up 1 buffer byte. Round up.
  //
  BufferLength = (HexCnt + 1) / 2; 

  //
  // Test if  buffer is passed enough.
  //
  if (BufferLength > (*Len)) {
    *Len = BufferLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  *Len = BufferLength;

  for (Idx = 0; Idx < HexCnt; Idx++) {

    R8_IsHexDigit (&Digit, Str[HexCnt - 1 - Idx]);

    //
    // For odd charaters, write the lower nibble for each buffer byte,
    // and for even characters, the upper nibble.
    //
    if ((Idx & 1) == 0) {
      Byte = Digit;
    } else {
      Byte = Buf[Idx / 2];
      Byte &= 0x0F;
      Byte = (UINT8) (Byte | Digit << 4);
    }

    Buf[Idx / 2] = Byte;
  }

  if (ConvertedStrLen != NULL) {
    *ConvertedStrLen = HexCnt;
  }

  return EFI_SUCCESS;
}
////~

////
BOOLEAN
R8_IsHexDigit (
  OUT UINT8      *Digit,
  IN  CHAR16      Char
  )
/*++

  Routine Description:
    Determines if a Unicode character is a hexadecimal digit.
    The test is case insensitive.

  Arguments:
    Digit - Pointer to byte that receives the value of the hex character.
    Char  - Unicode character to test.

  Returns:
    TRUE  - If the character is a hexadecimal digit.
    FALSE - Otherwise.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //

  if ((Char >= L'0') && (Char <= L'9')) {
    *Digit = (UINT8) (Char - L'0');
    return TRUE;
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    *Digit = (UINT8) (Char - L'A' + 0x0A);
    return TRUE;
  }

  if ((Char >= L'a') && (Char <= L'f')) {
    *Digit = (UINT8) (Char - L'a' + 0x0A);
    return TRUE;
  }

  return FALSE;
}
////~

////
CHAR16
R8_NibbleToHexChar (
  IN UINT8      Nibble
  )
/*++

  Routine Description:
    Converts the low nibble of a byte  to hex unicode character.

  Arguments:
    Nibble - lower nibble of a byte.

  Returns:
    Hex unicode character.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //

  Nibble &= 0x0F;
  if (Nibble <= 0x9) {
    return (CHAR16)(Nibble + L'0');
  }

  return (CHAR16)(Nibble - 0xA + L'A');
}
////~

////#HobLib
VOID *
R8_GetHob (
  IN UINT16  Type,
  IN VOID    *HobStart
  )
/*++

Routine Description:

  This function returns the first instance of a HOB type in a HOB list.
  
Arguments:

  Type          - The HOB type to return.
  HobStart      - The first HOB in the HOB list.
    
Returns:

  HobStart      - There were no HOBs found with the requested type.
  Other         - The first HOB with the matching type.

--*/
{
  //
  // Porting Guide:
  // Edk II HobLib GetNextHob () is an equivelent function with the following exceptions:
  // 1. GetNextHob () does not allow NULL value as the argument of HobStart by ASSERT ()  
  // 2. GetNextHob () will return NULL instead of returning HobStart when such kind of
  //    HOB can be retrieved, so caller does not need to re-check the return HOB type any longer.
  //

  VOID    *Hob;
  //
  // Return input if not found
  //
  if (HobStart == NULL) {
    return HobStart;
  }
  Hob = GetNextHob (Type, HobStart);
  if (Hob == NULL) {
    return HobStart;
  }
  
  return Hob;
}
////~

////
UINTN
R8_GetHobListSize (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get size of hob list.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Size of hob list.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Size;

  Hob.Raw = HobStart;
  Size    = 0;

  while (Hob.Header->HobType != EFI_HOB_TYPE_END_OF_HOB_LIST) {
    Size += Hob.Header->HobLength;
    Hob.Raw += Hob.Header->HobLength;
  }

  Size += Hob.Header->HobLength;

  return Size;
}
////~

////
UINT32
R8_GetHobVersion (
  IN VOID  *HobStart
  )
/*++

Routine Description:

  Get hob version.

Arguments:

  HobStart      - Start pointer of hob list

Returns:

  Hob version.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //

  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  return Hob.HandoffInformationTable->Version;
}
////~

////
EFI_STATUS
R8_GetHobBootMode (
  IN  VOID           *HobStart,
  OUT EFI_BOOT_MODE  *BootMode
  )
/*++

Routine Description:

  Get current boot mode.

Arguments:

  HobStart      - Start pointer of hob list
  BootMode      - Current boot mode recorded in PHIT hob

Returns:

  EFI_NOT_FOUND     - Invalid hob header
  EFI_SUCCESS       - Boot mode found

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  // In fact, since EFI_HANDOFF_HOB must be the first Hob,
  // the following code can retrieve boot mode.
  //
  // EFI_HOB_HANDOFF_INFO_TABLE *HandOffHob;
  //
  // HandOffHob = GetHobList ();  
  // ASSERT (HandOffHob->Header.HobType == EFI_HOB_TYPE_HANDOFF);
  // 
  // BootMode = HandOffHob->BootMode;
  //
  EFI_PEI_HOB_POINTERS  Hob;

  Hob.Raw = HobStart;
  if (Hob.Header->HobType != EFI_HOB_TYPE_HANDOFF) {
    return EFI_NOT_FOUND;
  }

  *BootMode = Hob.HandoffInformationTable->BootMode;
  return EFI_SUCCESS;
}
////~


////#HobLib
EFI_STATUS
R8_GetCpuHobInfo (
  IN  VOID   *HobStart,
  OUT UINT8  *SizeOfMemorySpace,
  OUT UINT8  *SizeOfIoSpace
  )
/*++

Routine Description:

  Get information recorded in CPU hob (Memory space size, Io space size)

Arguments:

  HobStart            - Start pointer of hob list
  SizeOfMemorySpace   - Size of memory size
  SizeOfIoSpace       - Size of IO size

Returns:

  EFI_NOT_FOUND     - CPU hob not found
  EFI_SUCCESS       - CPU hob found and information got.

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  // If Cpu HOB info is indispensable, user is able to ASSERT ()
  // first to save error handling code
  // For example:
  //
  // EFI_HOB_CPU  *CpuHob;
  // 
  // CpuHob = GetHob (EFI_HOB_TYPE_CPU, HobStart);
  // ASSERT (CpuHob != NULL);
  // 
  // ...
  // 
  EFI_HOB_CPU  *CpuHob;

  CpuHob = GetHob (EFI_HOB_TYPE_CPU, HobStart);
  if (CpuHob == NULL) {
    return EFI_NOT_FOUND;
  }

  *SizeOfMemorySpace  = CpuHob->SizeOfMemorySpace;
  *SizeOfIoSpace      = CpuHob->SizeOfIoSpace;
  return EFI_SUCCESS;
}
////~

////#HobLib
EFI_STATUS
R8_GetDxeCoreHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length,
  OUT VOID                  **EntryPoint,
  OUT EFI_GUID              **FileName
  )
/*++

Routine Description:

  Get memory allocation hob created for DXE core and extract its information

Arguments:

  HobStart        - Start pointer of the hob list
  BaseAddress     - Start address of memory allocated for DXE core
  Length          - Length of memory allocated for DXE core
  EntryPoint      - DXE core file name
  FileName        - File Name

Returns:

  EFI_NOT_FOUND   - DxeCoreHob not found  
  EFI_SUCCESS     - DxeCoreHob found and information got

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  // 
  EFI_PEI_HOB_POINTERS  DxeCoreHob;
  
  for (DxeCoreHob.Raw = HobStart; 
      (DxeCoreHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, DxeCoreHob.Raw)) != NULL;
       DxeCoreHob.Raw = GET_NEXT_HOB (DxeCoreHob)) {
    if (CompareGuid (&DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.Name, 
                      &gEfiHobMemeryAllocModuleGuid)) {
        *BaseAddress  = DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryBaseAddress;
        *Length       = DxeCoreHob.MemoryAllocationModule->MemoryAllocationHeader.MemoryLength;
        *EntryPoint   = (VOID *) (UINTN) DxeCoreHob.MemoryAllocationModule->EntryPoint;
        *FileName     = &DxeCoreHob.MemoryAllocationModule->ModuleName;
        return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
////~

////#HobLib
EFI_STATUS
R8_GetNextFirmwareVolumeHob (
  IN OUT VOID                  **HobStart,
  OUT    EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT    UINT64                *Length
  )
/*++

Routine Description:

  Get next firmware volume hob from HobStart

Arguments:

  HobStart        - Start pointer of hob list
  BaseAddress     - Start address of next firmware volume
  Length          - Length of next firmware volume

Returns:

  EFI_NOT_FOUND   - Next firmware volume not found
  EFI_SUCCESS     - Next firmware volume found with address information

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  // Pay attention that caller is REQUIRED to update HobStart with:
  // *HobStart =  GET_NEXT_HOB (FirmwareVolumeHob)
  //
  // If FV HOB info is indispensable, user is able to ASSERT ()
  // first to save error handling code
  // For example:
  //
  // EFI_HOB_FIRMWARE_VOLUME  *FirmwareVolumeHob;
  // 
  // FirmwareVolumeHob = GetHob (EFI_HOB_TYPE_FV, HobStart);
  // ASSERT (FirmwareVolumeHob != NULL);
  // 
  // ...
  //

  EFI_PEI_HOB_POINTERS  FirmwareVolumeHob;

  FirmwareVolumeHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, *HobStart);
  if (FirmwareVolumeHob.Raw != NULL) {
    return EFI_NOT_FOUND;
  }

  *BaseAddress  = FirmwareVolumeHob.FirmwareVolume->BaseAddress;
  *Length       = FirmwareVolumeHob.FirmwareVolume->Length;

  *HobStart     = GET_NEXT_HOB (FirmwareVolumeHob);

  return EFI_SUCCESS;
}
////~

////#HobLib
EFI_STATUS
R8_GetNextGuidHob (
  IN OUT VOID      **HobStart,
  IN     EFI_GUID  * Guid,
  OUT    VOID      **Buffer,
  OUT    UINTN     *BufferSize OPTIONAL
  )
/*++

Routine Description:
  Get the next guid hob.
  
Arguments:
  HobStart        - A pointer to the start hob.
  Guid            - A pointer to a guid.
  Buffer          - A pointer to the buffer.
  BufferSize      - Buffer size.
  
Returns:
  EFI_NOT_FOUND          - Next Guid hob not found
  EFI_SUCCESS            - Next Guid hob found and data for this Guid got
  EFI_INVALID_PARAMETER  - invalid parameter

--*/
{
  //
  // Porting Guide:
  // This library interface is changed substantially with R9 counerpart GetNextGuidHob (). 
  // 1. R9 GetNextGuidHob has two parameters and returns the matched GUID HOB from the StartHob. 
  // 2. R9 GetNextGuidHob does not strip the HOB header, so caller is required to apply
  //    GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE () to extract the data section and its
  //    size info respectively.
  // 3. this function does not skip the starting HOB pointer unconditionally:
  //    it returns HobStart back if HobStart itself meets the requirement;
  //    caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  //
  EFI_PEI_HOB_POINTERS  GuidHob;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob.Raw = GetNextGuidHob (Guid, *HobStart);
  if (GuidHob.Raw == NULL) {
    return EFI_NOT_FOUND;
  }
  
  *Buffer = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (BufferSize != NULL) {
    *BufferSize = GET_GUID_HOB_DATA_SIZE (GuidHob.Guid);
  }

  *HobStart = GET_NEXT_HOB (GuidHob);

  return EFI_SUCCESS;
}
////~

////#HobLib
EFI_STATUS
R8_GetPalEntryHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *PalEntry
  )
/*++

Routine Description:

  Get PAL entry from PalEntryHob

Arguments:

  HobStart      - Start pointer of hob list
  PalEntry      - Pointer to PAL entry

Returns:

  Status code.

--*/
{
  EFI_HOB_GUID_TYPE   *GuidHob;

  GuidHob = GetNextGuidHob (&gPalEntryHob, HobStart);

  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  *PalEntry = *((EFI_PHYSICAL_ADDRESS *) GET_GUID_HOB_DATA (GuidHob));
  return EFI_SUCCESS;
}
////~

////#HobLib
EFI_STATUS
R8_GetIoPortSpaceAddressHobInfo (
  IN  VOID                  *HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *IoPortSpaceAddress
  )
/*++

Routine Description:

  Get IO port space address from IoBaseHob.

Arguments:

  HobStart              - Start pointer of hob list
  IoPortSpaceAddress    - IO port space address

Returns:

  Status code

--*/
{
  //
  // Porting Guide:
  // This library interface is simply obsolete. 
  // Include the source code to user code.
  //
  EFI_HOB_GUID_TYPE   *GuidHob;

  GuidHob = GetNextGuidHob (&gEfiIoBaseHobGuid, HobStart);

  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  *IoPortSpaceAddress = *((EFI_PHYSICAL_ADDRESS *) GET_GUID_HOB_DATA (GuidHob));
  return EFI_SUCCESS;
}
////~

////#HobLib
EFI_STATUS
R8_PeiBuildHobGuid (
  IN  EFI_GUID          *Guid,
  IN  UINTN             DataLength,
  OUT VOID              **Hob
  )
/*++

Routine Description:

  Builds a custom HOB that is tagged with a GUID for identification

Arguments:

  Guid        - The GUID of the custome HOB type
  DataLength  - The size of the data payload for the GUIDed HOB
  Hob         - Pointer to pointer to the created Hob

Returns:

  EFI_SUCCESS - Hob is successfully built.
  Others      - Errors occur while creating new Hob

--*/
{
  //
  // Porting Guide: Apply the new interface of BuildGuidHob in R9 HobLib.
  // Pay attention that the return value has been changed to the start address of
  // GUID HOB data so that caller can fill the customized data. 
  // For BuildGuidHob (), the HOB Header and Name field is already stripped..
  //
  VOID   *HobData; 

  HobData = BuildGuidHob (Guid, DataLength);
  //
  // This step is necessary to be compatible with R8 interface!
  //
  *Hob    = (VOID *) ((UINT8 *) HobData - sizeof (EFI_HOB_GUID_TYPE)); 

  return EFI_SUCCESS; 
}
////~
