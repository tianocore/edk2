/** @file

Copyright (c) 2007 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcSupport.c

Abstract:

  Support routines for PxeBc


**/


#include "PxeBcImpl.h"


/**

  @param  Smbios              Pointer to SMBIOS structure
  @param  StringNumber        String number to return. 0 is used to skip all
                              strings and  point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == 0

**/
// GC_NOTO: function comment is missing 'Routine Description:'
CHAR8 *
GetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER  *Smbios,
  IN  UINT16                    StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;

  //
  // Skip over formatted section
  //
  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber || StringNumber == 0; Index++) {
    if (StringNumber == Index) {
      return String;
    }
    //
    // Skip string
    //
    for (; *String != 0; String++)
      ;
    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Return pointer to next structure in Smbios.
      //  if you pass in a 0 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}


/**
  This function gets system guid and serial number from the smbios table

  @param  SystemGuid          The pointer of returned system guid
  @param  SystemSerialNumber  The pointer of returned system serial number

  @retval EFI_SUCCESS         Successfully get the system guid and system serial
                              number
  @retval EFI_NOT_FOUND       Not find the SMBIOS table

**/
EFI_STATUS
GetSmbiosSystemGuidAndSerialNumber (
  IN  EFI_GUID  *SystemGuid,
  OUT CHAR8     **SystemSerialNumber
  )
{
  EFI_STATUS                Status;
  SMBIOS_TABLE_ENTRY_POINT  *SmbiosTable;
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT16                    Index;

  Status = EfiGetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID **) &SmbiosTable);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Smbios.Hdr    = (SMBIOS_STRUCTURE *) (UINTN) SmbiosTable->TableAddress;
  SmbiosEnd.Raw = (UINT8 *) (UINTN) (SmbiosTable->TableAddress + SmbiosTable->TableLength);

  for (Index = 0; Index < SmbiosTable->TableLength; Index++) {
    if (Smbios.Hdr->Type == 1) {
      if (Smbios.Hdr->Length < 0x19) {
        //
        // Older version did not support Guid and Serial number
        //
        continue;
      }
      //
      // SMBIOS tables are byte packed so we need to do a byte copy to
      // prevend alignment faults on Itanium-based platform.
      //
      CopyMem (SystemGuid, &Smbios.Type1->Uuid, sizeof (EFI_GUID));
      *SystemSerialNumber = GetSmbiosString (&Smbios, Smbios.Type1->SerialNumber);

      return EFI_SUCCESS;
    }
    //
    // Make Smbios point to the next record
    //
    GetSmbiosString (&Smbios, 0);

    if (Smbios.Raw >= SmbiosEnd.Raw) {
      //
      // SMBIOS 2.1 incorrectly stated the length of SmbiosTable as 0x1e.
      // given this we must double check against the length of the structure.
      //
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}


/**
  GC_NOTO: Add function description

  @param  Event               GC_NOTO: add argument description
  @param  Context             GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
VOID
PxeBcCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
}

EFI_STATUS
PxeBcConfigureUdpWriteInstance (
  IN EFI_UDP4_PROTOCOL  *Udp4,
  IN EFI_IPv4_ADDRESS   *StationIp,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *Gateway,
  IN OUT UINT16         *SrcPort
  )
{
  EFI_UDP4_CONFIG_DATA  Udp4CfgData;
  EFI_STATUS            Status;

  ZeroMem (&Udp4CfgData, sizeof (Udp4CfgData));

  Udp4CfgData.ReceiveTimeout = 1000;
  Udp4CfgData.TypeOfService  = DEFAULT_ToS;
  Udp4CfgData.TimeToLive     = DEFAULT_TTL;

  CopyMem (&Udp4CfgData.StationAddress, StationIp, sizeof (*StationIp));
  CopyMem (&Udp4CfgData.SubnetMask, SubnetMask, sizeof (*SubnetMask));

  Udp4CfgData.StationPort    = *SrcPort;

  //
  // Reset the instance.
  //
  Udp4->Configure (Udp4, NULL);

  Status = Udp4->Configure (Udp4, &Udp4CfgData);
  if (!EFI_ERROR (Status) && (Gateway->Addr[0] != 0)) {
    //
    // basic configuration OK, need to add the default route entry
    //
    Status = Udp4->Routes (Udp4, FALSE, &mZeroIp4Addr, &mZeroIp4Addr, Gateway);
    if (EFI_ERROR (Status)) {
      //
      // roll back
      //
      Udp4->Configure (Udp4, NULL);
    }
  }

  if (!EFI_ERROR (Status) && (*SrcPort == 0)) {
    Udp4->GetModeData (Udp4, &Udp4CfgData, NULL, NULL, NULL);
    *SrcPort = Udp4CfgData.StationPort;
  }

  return Status;
}


/**
  Convert number to ASCII value

  @param  Number              Numeric value to convert to decimal ASCII value.
  @param  Buffer              Buffer to place ASCII version of the Number
  @param  Length              Length of Buffer.

  @retval none                none

**/
VOID
CvtNum (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN INTN   Length
  )
{
  UINTN Remainder;

  while (Length--) {
    Remainder = Number % 10;
    Number /= 10;
    Buffer[Length] = (UINT8) ('0' + Remainder);
  }
}


/**
  GC_NOTO: Add function description

  @param  Number              GC_NOTO: add argument description
  @param  Buffer              GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
UINTN
UtoA10 (
  IN UINTN Number,
  IN CHAR8 *Buffer
  )
{
  UINTN Index;
  CHAR8 TempStr[64];

  Index           = 63;
  TempStr[Index]  = 0;

  do {
    Index--;
    TempStr[Index]  = (CHAR8) ('0' + (Number % 10));
    Number          = Number / 10;
  } while (Number != 0);

  AsciiStrCpy (Buffer, &TempStr[Index]);

  return AsciiStrLen (Buffer);
}


/**
  Convert ASCII numeric string to a UINTN value

  @param  Number              Numeric value to convert to decimal ASCII value.
  @param  Buffer              Buffer to place ASCII version of the Number

  @retval Value               UINTN value of the ASCII string.

**/
UINT64
AtoU64 (
  IN UINT8 *Buffer
  )
{
  UINT64  Value;
  UINT8   Character;

  Value = 0;
  while ((Character = *Buffer++) != '\0') {
    Value = MultU64x32 (Value, 10) + (Character - '0');
  }

  return Value;
}

