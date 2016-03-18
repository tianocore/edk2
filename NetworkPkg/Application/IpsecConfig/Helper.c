/** @file
  The assistant function implementation for IpSecConfig application.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecConfig.h"
#include "Helper.h"

/**
  Helper function called to change an input parameter in the string format to a number.

  @param[in]      FlagStr         The pointer to the flag string.
  @param[in]      Maximum         Greatest value number.
  @param[in, out] ValuePtr        The pointer to the input parameter in string format.
  @param[in]      ByteCount       The valid byte count
  @param[in]      Map             The pointer to the STR2INT table.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.
  @param[in]      FormatMask      The bit mask.
                                  BIT 0 set indicates the value of a flag might be a number.
                                  BIT 1 set indicates the value of a flag might be a string that needs to be looked up.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_NOT_FOUND            The input parameter can't be found.
  @retval EFI_INVALID_PARAMETER    The input parameter is an invalid input.
**/
EFI_STATUS
GetNumber (
  IN     CHAR16        *FlagStr,
  IN     UINT64        Maximum,
  IN OUT VOID          *ValuePtr,
  IN     UINTN         ByteCount,
  IN     STR2INT       *Map,
  IN     LIST_ENTRY    *ParamPackage,
  IN     UINT32        FormatMask
  )
{
  EFI_STATUS      Status;
  UINT64          Value64;
  BOOLEAN         Converted;
  UINTN           Index;
  CONST CHAR16    *ValueStr;

  ASSERT (FormatMask & (FORMAT_NUMBER | FORMAT_STRING));

  Converted = FALSE;
  Value64   = 0;
  ValueStr  = ShellCommandLineGetValue (ParamPackage, FlagStr);

  if (ValueStr == NULL) {
    return EFI_NOT_FOUND;
  } else {
    //
    // Try to convert to integer directly if MaybeNumber is TRUE.
    //
    if ((FormatMask & FORMAT_NUMBER) != 0) {
      Value64 = StrToUInteger (ValueStr, &Status);
      if (!EFI_ERROR (Status)) {
        //
        // Convert successfully.
        //
        if (Value64 > Maximum) {
          //
          // But the result is invalid
          //
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
            mHiiHandle,
            mAppName,
            FlagStr,
            ValueStr
            );
          return EFI_INVALID_PARAMETER;
        }

        Converted = TRUE;
      }
    }

    if (!Converted && ((FormatMask & FORMAT_STRING) != 0)) {
      //
      // Convert falied, so use String->Integer map.
      //
      ASSERT (Map != NULL);
      Value64 = MapStringToInteger (ValueStr, Map);
      if (Value64 == (UINT32) -1) {
        //
        // Cannot find the string in the map.
        //
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_IPSEC_CONFIG_INCORRECT_PARAMETER_VALUE),
          mHiiHandle,
          mAppName,
          FlagStr,
          ValueStr
          );
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_IPSEC_CONFIG_ACCEPT_PARAMETERS), mHiiHandle);
        for (Index = 0; Map[Index].String != NULL; Index++) {
          Print (L" %s", Map[Index].String);
        }

        Print (L"\n");
        return EFI_INVALID_PARAMETER;
      }
    }

    CopyMem (ValuePtr, &Value64, ByteCount);
    return EFI_SUCCESS;
  }
}

/**
  Helper function called to convert a string containing an Ipv4 or Ipv6 Internet Protocol address
  into a proper address for the EFI_IP_ADDRESS structure.

  @param[in]  Ptr    The pointer to the string containing an Ipv4 or Ipv6 Internet Protocol address.
  @param[out] Ip     The pointer to the EFI_IP_ADDRESS structure to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
**/
EFI_STATUS
EfiInetAddr2 (
  IN  CHAR16            *Ptr,
  OUT EFI_IP_ADDRESS    *Ip
  )
{
  EFI_STATUS    Status;

  if ((Ptr == NULL) || (Ip == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse the input address as Ipv4 Address first.
  //
  Status = NetLibStrToIp4 (Ptr, &Ip->v4);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = NetLibStrToIp6 (Ptr, &Ip->v6);
  return Status;
}

/**
  Helper function called to calculate the prefix length associated with the string
  containing an Ipv4 or Ipv6 Internet Protocol address.

  @param[in]  Ptr     The pointer to the string containing an Ipv4 or Ipv6 Internet Protocol address.
  @param[out] Addr    The pointer to the EFI_IP_ADDRESS_INFO structure to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval Others                   Other mistake case.
**/
EFI_STATUS
EfiInetAddrRange (
  IN  CHAR16                 *Ptr,
  OUT EFI_IP_ADDRESS_INFO    *Addr
  )
{
  EFI_STATUS    Status;

  if ((Ptr == NULL) || (Addr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = NetLibStrToIp4 (Ptr, &Addr->Address.v4);
  if (!EFI_ERROR (Status)) {
    if ((UINT32)(*Addr->Address.v4.Addr) == 0) {
      Addr->PrefixLength = 0;
    } else {
      Addr->PrefixLength = 32;
    }
    return Status;
  }

  Status = NetLibStrToIp6andPrefix (Ptr, &Addr->Address.v6, &Addr->PrefixLength);
  if (!EFI_ERROR (Status) && (Addr->PrefixLength == 0xFF)) {
    Addr->PrefixLength = 128;
  }

  return Status;
}

/**
  Helper function called to calculate the port range associated with the string.

  @param[in]  Ptr          The pointer to the string containing a port and range.
  @param[out] Port         The pointer to the Port to contain the result.
  @param[out] PortRange    The pointer to the PortRange to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval Others                   Other mistake case.
**/
EFI_STATUS
EfiInetPortRange (
  IN  CHAR16    *Ptr,
  OUT UINT16    *Port,
  OUT UINT16    *PortRange
  )
{
  CHAR16        *BreakPtr;
  CHAR16        Ch;
  EFI_STATUS    Status;

  for (BreakPtr = Ptr; (*BreakPtr != L'\0') && (*BreakPtr != L':'); BreakPtr++) {
    ;
  }

  Ch        = *BreakPtr;
  *BreakPtr = L'\0';
  *Port     = (UINT16) StrToUInteger (Ptr, &Status);
  *BreakPtr = Ch;
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *PortRange = 0;
  if (*BreakPtr == L':') {
    BreakPtr++;
    *PortRange = (UINT16) StrToUInteger (BreakPtr, &Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (*PortRange < *Port) {
      return EFI_INVALID_PARAMETER;
    }

    *PortRange = (UINT16) (*PortRange - *Port);
  }

  return EFI_SUCCESS;
}

/**
  Helper function called to transfer a string to an unsigned integer.

  @param[in]  Str       The pointer to the string.
  @param[out] Status    The operation status.

  @return The integer value of converted Str.
**/
UINT64
StrToUInteger (
  IN  CONST CHAR16    *Str,
  OUT EFI_STATUS      *Status
  )
{
  UINT64    Value;
  UINT64    NewValue;
  CHAR16    *StrTail;
  CHAR16    Char;
  UINTN     Base;
  UINTN     Len;

  Base    = 10;
  Value   = 0;
  *Status = EFI_ABORTED;

  //
  // Skip leading white space.
  //
  while ((*Str != 0) && (*Str == ' ')) {
    Str++;
  }
  //
  // For NULL Str, just return.
  //
  if (*Str == 0) {
    return 0;
  }
  //
  // Skip white space in tail.
  //
  Len     = StrLen (Str);
  StrTail = (CHAR16 *) (Str + Len - 1);
  while (*StrTail == ' ') {
    *StrTail = 0;
    StrTail--;
  }

  Len = StrTail - Str + 1;

  //
  // Check hex prefix '0x'.
  //
  if ((Len >= 2) && (*Str == '0') && ((*(Str + 1) == 'x') || (*(Str + 1) == 'X'))) {
    Str += 2;
    Len -= 2;
    Base = 16;
  }

  if (Len == 0) {
    return 0;
  }
  //
  // Convert the string to value.
  //
  for (; Str <= StrTail; Str++) {

    Char = *Str;

    if (Base == 16) {
      if (RShiftU64 (Value, 60) != 0) {
        //
        // Overflow here x16.
        //
        return 0;
      }

      NewValue = LShiftU64 (Value, 4);
    } else {
      if (RShiftU64 (Value, 61) != 0) {
        //
        // Overflow here x8.
        //
        return 0;
      }

      NewValue  = LShiftU64 (Value, 3);
      Value     = LShiftU64 (Value, 1);
      NewValue += Value;
      if (NewValue < Value) {
        //
        // Overflow here.
        //
        return 0;
      }
    }

    Value = NewValue;

    if ((Base == 16) && (Char >= 'a') && (Char <= 'f')) {
      Char = (CHAR16) (Char - 'a' + 'A');
    }

    if ((Base == 16) && (Char >= 'A') && (Char <= 'F')) {
      Value += (Char - 'A') + 10;
    } else if ((Char >= '0') && (Char <= '9')) {
      Value += (Char - '0');
    } else {
      //
      // Unexpected Char encountered.
      //
      return 0;
    }
  }

  *Status = EFI_SUCCESS;
  return Value;
}

/**
  Helper function called to transfer a string to an unsigned integer according to the map table.

  @param[in] Str    The pointer to the string.
  @param[in] Map    The pointer to the map table.

  @return The integer value of converted Str. If not found, then return -1.
**/
UINT32
MapStringToInteger (
  IN CONST CHAR16    *Str,
  IN STR2INT         *Map
  )
{
  STR2INT       *Item;

  for (Item = Map; Item->String != NULL; Item++) {
    if (StrCmp (Item->String, Str) == 0) {
      return Item->Integer;
    }
  }

  return (UINT32) -1;
}

/**
  Helper function called to transfer an unsigned integer to a string according to the map table.

  @param[in] Integer    The pointer to the string.
  @param[in] Map        The pointer to the map table.

  @return The converted Str. If not found, then return NULL.
**/
CHAR16 *
MapIntegerToString (
  IN UINT32     Integer,
  IN STR2INT    *Map
  )
{
  STR2INT    *Item;

  for (Item = Map; Item->String != NULL; Item++) {
    if (Integer == Item->Integer) {
      return Item->String;
    }
  }

  return NULL;
}
